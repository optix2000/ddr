#include <LiquidCrystal.h>
#include <HID-Project.h>
#include <HX711.h>


// Gamepad button ID
const byte BUTTON_UP = 1;
const byte BUTTON_DOWN = 2;
const byte BUTTON_LEFT = 3;
const byte BUTTON_RIGHT = 4;

// Load cell pins
const byte LC_UP_DOUT = 0;
const byte LC_UP_SCK = A2;
const byte LC_DOWN_DOUT = 1;
const byte LC_DOWN_SCK = A3;
const byte LC_LEFT_DOUT = 2;
const byte LC_LEFT_SCK = A4;
const byte LC_RIGHT_DOUT = 3;
const byte LC_RIGHT_SCK = A5;

// Loadcell params
const float LC_THRESHOLD = 1.10; // Multiplier above max seen value
long LC_STEP_THRESHOLDS[] = {0, 0, 0, 0};

// LCD            RS,E,d4,d5,d6,d7
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Pad
class Pad {
  private:
    String text;
    byte lc_dout;
    byte lc_sck;
    HX711 hx711;
    long step_threshold = 0;
    byte button_id;
  public:
    Pad(String text, byte button_id, byte lc_dout, byte lc_sck);
    String name();
    byte buttonId();
    void calibrate();
    void setInterrupt();
    void handle();
    HX711 lc();
};

Pad::Pad(String text, byte button_id, byte lc_dout, byte lc_sck) : text(text), button_id(button_id), lc_dout(lc_dout), lc_sck(lc_sck) {
  hx711.begin(lc_dout, lc_sck);
}
String Pad::name() {
  return text;
}
byte Pad::buttonId() {
  return button_id;
}
void Pad::handle() {
  long reading = hx711.get_value();
  if (reading > step_threshold) {
    Gamepad.press(button_id);
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Gamepad.release(button_id);
    digitalWrite(LED_BUILTIN, LOW);
  }
  Gamepad.write();
}
HX711 Pad::lc() {
  return hx711;
}

void Pad::calibrate() {
    hx711.tare(100);

    long max_val = 0;
    for (byte i = 0; i < 100; i++) {
      long reading = hx711.get_value();
      if (reading > max_val) {
        max_val = reading;
      }
    }
    step_threshold = max_val * LC_THRESHOLD;
}

Pad up("Up", BUTTON_UP, LC_UP_DOUT, LC_UP_SCK);
Pad down("Down", BUTTON_DOWN, LC_DOWN_DOUT, LC_DOWN_SCK);
Pad left("Left", BUTTON_LEFT, LC_LEFT_DOUT, LC_LEFT_SCK);
Pad right("Right", BUTTON_RIGHT, LC_RIGHT_DOUT, LC_RIGHT_SCK);

const byte NUM_PADS = 4;
const Pad all_pads[] = {up, down, left, right};

void dPrint(String msg) {
  if (Serial) {
    Serial.println(msg);
  }
  lcd.print(msg);
}

// Call if unrecoverable failure
void die() {
  while(true){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

bool selfTest() {
  for (byte i = 0; i < NUM_PADS; i++) {
    Pad pad = all_pads[i];
    if (pad.lc().wait_ready_timeout(1000, 100)) {
      long reading = pad.lc().get_value(100);
      String output = "LC ";
      output += pad.name();
      output += " Reading: ";
      output += reading;
      dPrint(output);
    } else {
      String output = "Failed LC ";
      output += pad.name();
      dPrint(output);
      return false;
    }
  }
  return true;
}

void calibrate() {
  for (byte i = 0; i < NUM_PADS; i++) {
    all_pads[i].calibrate();
  }
}

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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600); // Debugging
  lcd.begin(16, 2); // 16x2 1602 LCD
  Gamepad.begin();

  dPrint("Initializing...");


  if (!selfTest()) die();

  dPrint("Calibrating. Do not touch");
  calibrate();
  dPrint("Configuring");

  attachInterrupt(digitalPinToInterrupt(LC_UP_DOUT), handleUp, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_DOWN_DOUT), handleDown, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_LEFT_DOUT), handleLeft, LOW);
  attachInterrupt(digitalPinToInterrupt(LC_RIGHT_DOUT), handleRight, LOW);

  dPrint("Ready!");
}

// the loop function runs over and over again forever
void loop() {
  while(true) {
    Gamepad.press(BUTTON_UP);
    Gamepad.write();
  }
}
