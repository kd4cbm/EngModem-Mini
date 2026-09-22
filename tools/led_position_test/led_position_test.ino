// EngModem Mini (Rev5) - front-panel LED position test
//
// Steps through the 8 LEDs in the order the DESIGN says they sit, left to right,
// blinking each one for 3 seconds while the VFD shows:
//   row 1: "Position N of 8"  (+ ON/OFF for driven LEDs, or the live pin level)
//   row 2: the LED's label and function
// You watch which physical LED blinks and compare it with position N.
//
// LED wiring facts used here (from the Rev5 netlist; every LED lights when its
// drive signal is HIGH - anode -> resistor -> signal, cathode -> GND):
//   Direct GPIO : HS = GPIO11, AA = GPIO10, OH = GPIO12
//   Via U4 (74HCT245, non-inverting): the LED follows a TTL signal net
//     MR = /TTL-DSR  (GPIO7,  ESP output)  -> can be driven
//     SD = /TTL-TXD  (GPIO16, ESP output)  -> can be driven
//     CD = /TTL-DCD  (GPIO5,  ESP output)  -> can be driven
//     TR = /TTL-DTR  (GPIO4,  driven by the MAX3237 from the PC's DTR)  -> NOT driven
//     RD = /TTL-RXD  (GPIO15, driven by the MAX3237 from the PC's TXD)  -> NOT driven
// TR and RD nets are OUTPUTS of the MAX3237, so the ESP must not fight them: those
// two pins are left as inputs and their slots only show the live pin level.
// They stay lit (idle high) unless the PC asserts DTR / sends data, so:
//   - open a terminal on the modem COM port: TR (DTR) goes OFF
//   - type characters in it: RD flickers
//
// Runs forever (repeats after position 8). Touches nothing but GPIO and the VFD:
// no WiFi, no SPIFFS, no saved config. Reflash the normal firmware afterwards.

#include <Arduino.h>

// Set to 1 AFTER U4 (74HCT245) has been replaced with an inverting buffer such as the 74HCT640: the LEDs
// that go through U4 (MR, TR, SD, RD, CD) then light when the ESP32 pin is LOW. (Untested on hardware -
// written for the planned swap; direct-drive LEDs OH, AA and HS are never affected.)
#define U4_INVERTING 0

// --- VFD (same pins and init sequence as the real firmware, vfd.ino) ---
#define VFD_RS  42
#define VFD_E   41
#define VFD_DB4 40
#define VFD_DB5 39
#define VFD_DB6 8
#define VFD_DB7 9
#define VFD_COLS 24

static void vfdPulseEnable() {
  digitalWrite(VFD_E, HIGH); delayMicroseconds(1);
  digitalWrite(VFD_E, LOW);  delayMicroseconds(1);
}
static void vfdWriteNibble(uint8_t n) {
  digitalWrite(VFD_DB7, (n >> 3) & 1);
  digitalWrite(VFD_DB6, (n >> 2) & 1);
  digitalWrite(VFD_DB5, (n >> 1) & 1);
  digitalWrite(VFD_DB4, (n >> 0) & 1);
  vfdPulseEnable();
}
static void vfdWriteByte(uint8_t rs, uint8_t v) {
  digitalWrite(VFD_RS, rs ? HIGH : LOW);
  delayMicroseconds(1);
  vfdWriteNibble((v >> 4) & 0x0F);
  vfdWriteNibble(v & 0x0F);
  if (rs == 0 && v <= 0x03) delayMicroseconds(100); else delayMicroseconds(1);
}
static void vfdSequence() {          // the firmware's init, minus its 250 ms power-up wait
  vfdWriteNibble(0b0010);
  delayMicroseconds(100);
  vfdWriteByte(0, 0x20);             // Function set (4-bit)
  vfdWriteByte(0, 0x0C);             // Display on, cursor off
  vfdWriteByte(0, 0x06);             // Entry mode
  vfdWriteByte(0, 0x01);             // Clear
}
static void vfdRow(uint8_t row, const char *text) {   // writes exactly 24 columns, space-padded
  vfdWriteByte(0, row ? 0xC0 : 0x80);
  bool end = false;
  for (int i = 0; i < VFD_COLS; i++) {
    char c = end ? ' ' : text[i];
    if (c == 0) { end = true; c = ' '; }
    vfdWriteByte(1, (uint8_t)c);
  }
}

// --- LED table, in DESIGN position order (position 1 = leftmost) ---
enum Kind { DRIVEN, HW_INPUT };
struct Led {
  const char *row2;       // label + function, max 24 chars
  int pin;                // GPIO that makes the LED light when HIGH (or that we read)
  Kind kind;
  const char *sigName;    // shown for HW_INPUT slots
  bool viaU4;             // true if the LED is fed through U4 (affected by U4_INVERTING)
};
static const Led LEDS[8] = {
  { "MR - Modem Ready (DSR)",  7,  DRIVEN,   "",     true  },
  { "TR - Term. Ready (DTR)",  4,  HW_INPUT, "DTR",  true  },
  { "SD - Send Data (TXD)",    16, DRIVEN,   "",     true  },
  { "RD - Recv Data (RXD)",    15, HW_INPUT, "RXD",  true  },
  { "OH - Off Hook",           12, DRIVEN,   "",     false },
  { "CD - Carrier Det (DCD)",  5,  DRIVEN,   "",     true  },
  { "AA - Auto Answer",        10, DRIVEN,   "",     false },
  { "HS - High Speed",         11, DRIVEN,   "",     false },
};

#define SLOT_MS   3000UL
#define BLINK_MS  250UL       // 2 Hz blink, so a driven LED is distinguishable from a steady one

// Light or darken a driven LED, allowing for an inverting U4.
static void drive(int idx, bool on) {
  bool level = on;
  if (LEDS[idx].viaU4 && U4_INVERTING) level = !level;
  digitalWrite(LEDS[idx].pin, level ? HIGH : LOW);
}

static void allDrivenOff() {
  for (int i = 0; i < 8; i++)
    if (LEDS[i].kind == DRIVEN) drive(i, false);
}

static void show(int idx, bool on) {
  char r1[VFD_COLS + 1];
  if (LEDS[idx].kind == DRIVEN)
    snprintf(r1, sizeof r1, "Position %d of 8  %s", idx + 1, on ? "ON " : "OFF");
  else
    snprintf(r1, sizeof r1, "Position %d of 8  %s:%c", idx + 1, LEDS[idx].sigName,
             digitalRead(LEDS[idx].pin) ? 'H' : 'L');
  vfdRow(0, r1);
  vfdRow(1, LEDS[idx].row2);
}

void setup() {
  Serial0.begin(115200);                       // debug UART on J7 (optional)

  // VFD pins
  const int vp[] = { VFD_RS, VFD_E, VFD_DB4, VFD_DB5, VFD_DB6, VFD_DB7 };
  for (int p : vp) pinMode(p, OUTPUT);
  digitalWrite(VFD_E, LOW); digitalWrite(VFD_RS, LOW);
  delay(250);
  vfdSequence();

  // LED drive pins (all are outputs in the normal firmware too)
  for (int i = 0; i < 8; i++)
    if (LEDS[i].kind == DRIVEN) { pinMode(LEDS[i].pin, OUTPUT); drive(i, false); }
  // MAX3237-driven nets: inputs only, never driven
  pinMode(4,  INPUT);   // /TTL-DTR
  pinMode(15, INPUT);   // /TTL-RXD
  // keep the other modem lines at their normal firmware idle levels
  pinMode(6,  OUTPUT); digitalWrite(6,  HIGH);   // RI inactive
  pinMode(18, OUTPUT); digitalWrite(18, LOW);    // RTS asserted (PC's CTS)
  pinMode(17, INPUT);                            // CTS-in (PC's RTS)

  Serial0.println("\r\nEngModem Mini LED position test - watch the front panel, left to right");
}

void loop() {
  vfdSequence();                               // re-send init each round in case the first was missed
  vfdRow(0, "LED position test"); vfdRow(1, "Watch left to right");
  allDrivenOff();
  delay(2000);

  for (int i = 0; i < 8; i++) {
    Serial0.printf("Position %d: %s\r\n", i + 1, LEDS[i].row2);
    uint32_t start = millis();
    bool on = false;
    while (millis() - start < SLOT_MS) {
      on = !on;
      if (LEDS[i].kind == DRIVEN) drive(i, on);
      show(i, on);                             // refreshed every 250 ms (also recovers a lost write)
      delay(BLINK_MS);
    }
    allDrivenOff();
  }
  vfdRow(0, "Round complete"); vfdRow(1, "Repeating...");
  delay(1500);
}
