#include <Wire.h>

#define MAX30205_ADDR 0x48

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Serial.println("MAX30205 START");
}

float readTemp() {
  Wire.beginTransmission(MAX30205_ADDR);
  Wire.write(0x00);

  if (Wire.endTransmission(false) != 0) {
    return -100;
  }

  Wire.requestFrom(MAX30205_ADDR, (uint8_t)2);

  if (Wire.available() < 2) {
    return -100;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t raw = (msb << 8) | lsb;

  // REAL MAX30205 conversion
  raw = raw >> 4;
  float tempC = raw * 0.0625;

  return tempC;
}

void loop() {
  float tempC = readTemp();

  if (tempC == -100) {
    Serial.println("Sensor read failed");
  } 
  else {
    Serial.print("Temperature: ");
    Serial.print(tempC, 2);
    Serial.println(" °C");
  }

  delay(1000);
}