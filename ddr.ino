#include <LiquidCrystal.h>
#include <HID-Project.h>
#include <HX711.h>

// CONFIG

#define DEBUG 1

// Gamepad button ID
const byte BUTTON_UP = 1;
const byte BUTTON_DOWN = 2;
const byte BUTTON_LEFT = 3;
const byte BUTTON_RIGHT = 4;

// Load cell pins
// NB: DOUT pins need interrupts
const byte LC_UP_DOUT = 1;
const byte LC_UP_SCK = A3;
const byte LC_DOWN_DOUT = 0;
const byte LC_DOWN_SCK = A2;
const byte LC_LEFT_DOUT = 2;
const byte LC_LEFT_SCK = A4;
const byte LC_RIGHT_DOUT = 3;
const byte LC_RIGHT_SCK = A5;

// Loadcell params
const float LC_THRESHOLD = 9000;

// LCD            RS,E,d4,d5,d6,d7
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// END CONFIG

// Pad
class Pad {
  private:
    String text;
    byte lc_dout;
    byte lc_sck;
    HX711 hx711;
    long step_threshold = 0;
    byte button_id;
    bool is_pressed = false;
  public:
    Pad(String text, byte button_id, byte lc_dout, byte lc_sck);
    void begin();
    String name();
    void calibrate();
    void handle();
    void press();
    void release();
    bool isPressed();
    HX711 lc();
};

Pad::Pad(String text, byte button_id, byte lc_dout, byte lc_sck) : text(text), button_id(button_id), lc_dout(lc_dout), lc_sck(lc_sck) {
}

void Pad::begin() {
  hx711.begin(lc_dout, lc_sck);
}

String Pad::name() {
  return text;
}

void Pad::handle() {
  long reading = hx711.get_value();
  if (reading > LC_THRESHOLD || reading < -LC_THRESHOLD) {
    press();
  } else {
    release();
  }
  Gamepad.write();
}

HX711 Pad::lc() {
  return hx711;
}

void Pad::calibrate() {
    hx711.tare(100);

    //long max_val = 0;
    //for (byte i = 0; i < 100; i++) {
    //  long reading = hx711.get_value();
    //  if (reading > max_val) {
    //    max_val = reading;
    //  }
    //}
    //step_threshold = max_val * LC_THRESHOLD;
}

void Pad::press() {
  if (!is_pressed) {
    Gamepad.press(button_id);
    digitalWrite(LED_BUILTIN, HIGH);
    is_pressed = true;
    Gamepad.write();
  }
}

void Pad::release() {
  if (is_pressed) {
    Gamepad.release(button_id);
    digitalWrite(LED_BUILTIN, LOW);
    is_pressed = false;
    Gamepad.write();
  }
}

bool Pad::isPressed() {
  return is_pressed;
}

Pad up("Up", BUTTON_UP, LC_UP_DOUT, LC_UP_SCK);
Pad down("Down", BUTTON_DOWN, LC_DOWN_DOUT, LC_DOWN_SCK);
Pad left("Left", BUTTON_LEFT, LC_LEFT_DOUT, LC_LEFT_SCK);
Pad right("Right", BUTTON_RIGHT, LC_RIGHT_DOUT, LC_RIGHT_SCK);

const byte NUM_PADS = 4;
const Pad *all_pads[] = {&up, &down, &left, &right};

const byte LCD_UP = 0;
const byte LCD_DOWN = 1;
const byte LCD_LEFT = 127;
const byte LCD_RIGHT = 126;

const uint8_t up_char[] = {
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00000,
  0b00000,
};
const uint8_t down_char[] = {
  0b00000,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000,
  0b00000,
};

// Debug print. Try Serial if avail, otherwise print to LCD
void dPrint(String msg) {
  static char buffer[17]; // 16 col LCD + null byte

#if DEBUG == 1
  if (Serial) {
    Serial.println(msg);
  }
#endif

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(msg);
  msg.toCharArray(buffer, 17);
}

// Call if unrecoverable failure
void die() {
  while(true){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(75);
    digitalWrite(LED_BUILTIN, LOW);
    delay(75);
  }
}

bool selfTest() {
  for (byte i = 0; i < NUM_PADS; i++) {
    Pad *pad = all_pads[i];
    if (pad->lc().wait_ready_timeout(1000, 100)) {
      long reading = pad->lc().get_value();
      String output = "LC ";
      output += pad->name();
      output += " : ";
      output += reading;
      dPrint(output);
    } else {
      String output = pad->name();
      output += " Failed Test";
      dPrint(output);
      return false;
    }
  }
  return true;
}

// Calibrate all
void calibrate() {
  for (byte i = 0; i < NUM_PADS; i++) {
    dPrint("Calibrating " + all_pads[i]->name());
    all_pads[i]->calibrate();
  }
}

// HACK?: I think this is the only way of binding ISFs
void handleUp() {
  up.handle();
}
void handleDown() {
  down.handle();
}
void handleLeft() {
  left.handle();
}
void handleRight() {
  right.handle();
}

void printArrows() {
  bool changed = false;
  static bool prev_state[NUM_PADS];
  char arrow;
  for (byte i = 0; i < NUM_PADS; i++) {
    if (i == 0) { // Up
      lcd.setCursor(14, 0);
      arrow = LCD_UP;
    } else if (i == 1) { // Down
      lcd.setCursor(14, 1);
      arrow = LCD_DOWN;
    } else if (i == 2) { // Left
      lcd.setCursor(13, 1);
      arrow = LCD_LEFT;
    } else if (i == 3) { // Right
      lcd.setCursor(15, 1);
      arrow = LCD_RIGHT;
    }
    bool pressed = all_pads[i]->isPressed();
    if (prev_state[i] != all_pads[i]->isPressed()) { // If state changed
      prev_state[i] = pressed;
      if (pressed) {
        lcd.write(arrow);
      } else {
        lcd.write(32); // Blank char
      }
    }
  }
}

void menu() {
  return;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Backlight off
  //pinMode(10, OUTPUT);
  //digitalWrite(10, LOW);

  Serial.begin(9600); // Debugging
  lcd.begin(16, 2); // 16x2 1602 LCD

  dPrint("Initializing...");
  // Delay to open serial for debugging
  //delay(10000);

  Gamepad.begin();
  for (byte i = 0; i < NUM_PADS; i++) {
    all_pads[i]->begin();
  }

  dPrint("Self Test...");
  if (!selfTest()) die();

  dPrint("Calibrating...");
  calibrate();
  dPrint("Finalizing...");

  attachInterrupt(digitalPinToInterrupt(LC_UP_DOUT), handleUp, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_DOWN_DOUT), handleDown, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_LEFT_DOUT), handleLeft, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_RIGHT_DOUT), handleRight, LOW);


  lcd.createChar(0, up_char);
  lcd.createChar(1, down_char);

  dPrint("Ready!");
}

void loop() {
  while(true) {
    int button = analogRead(A0);
    if (button > 960) { // None
    } else if (button < 64) { // Right
      right.press();
    } else if (button < 192) { // Up
      up.press();
    } else if (button < 384) { // Down
      down.press();
    } else if (button < 512) { // Left
      left.press();
    } else if (button < 832) { // Select
      menu();
    }
    // LCD menu stuff here.
    printArrows();
  }
}
