#include <Wire.h>

#define DS3231_ADDRESS 0x68

byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // =====================================
  // SET YOUR DATE AND TIME HERE
  // =====================================

  byte second = 0;
  byte minute = 0;
  byte hour   = 19;  // 7:00 PM
  byte day    = 26;
  byte month  = 9;
  byte year   = 26;  // 2026

  // =====================================

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(0x00);

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(1));       // Day of week
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));

  Wire.endTransmission();

  Serial.println("DS3231 time updated!");
}

void loop() {
}