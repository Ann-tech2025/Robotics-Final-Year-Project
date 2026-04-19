#include <Wire.h>

#define SDA_PIN 25
#define SCL_PIN 26
#define MPU_addr 0x68

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
float ax, ay, az;
float gx, gy, gz;

bool fall = false;
bool trigger1 = false;
bool trigger2 = false;
bool trigger3 = false;

byte trigger1count = 0;
byte trigger2count = 0;
byte trigger3count = 0;

float angleChange = 0;

// Offsets
float offsetX = 0, offsetY = 0, offsetZ = 0;

// Time tracking
unsigned long startTime;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Starting MPU6050...");

  // Check connection
  Wire.beginTransmission(MPU_addr);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 NOT FOUND!");
    while (1);
  }

  // Wake up MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(100);

  // -------- Calibration --------
  Serial.println("Calibrating... Keep still");
  long sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 0; i < 200; i++) {
    mpu_read();
    sumX += AcX;
    sumY += AcY;
    sumZ += AcZ;
    delay(5);
  }

  offsetX = sumX / 200.0;
  offsetY = sumY / 200.0;
  offsetZ = sumZ / 200.0;

  Serial.println("Calibration Done");

  startTime = millis(); // start time for timestamp
}

void loop() {
  mpu_read();

  float t = (millis() - startTime) / 1000.0; // time in seconds

  // Apply offset (removes gravity bias)
  ax = (AcX - offsetX) / 16384.0;
  ay = (AcY - offsetY) / 16384.0;
  az = (AcZ - offsetZ) / 16384.0;

  gx = GyX / 131.0;
  gy = GyY / 131.0;
  gz = GyZ / 131.0;

  float Amp = sqrt(ax*ax + ay*ay + az*az)*10.0;

  // TRIGGER 1 (Free fall)
  if (Amp <= 2.0 && !trigger1 && !trigger2) {
    trigger1 = true;
    trigger1count = 0;
    Serial.println("TRIGGER 1");
  }

  if (trigger1) {
    trigger1count++;
    if (Amp >= 12.0) {
      trigger2 = true;
      trigger1 = false;
      trigger1count = 0;
      Serial.println("TRIGGER 2");
    }
    if (trigger1count >= 6) {
      trigger1 = false;
      trigger1count = 0;
    }
  }

  // TRIGGER 2 (Impact)
  if (trigger2) {
    trigger2count++;
    angleChange = sqrt(gx*gx + gy*gy + gz*gz);
    Serial.print("AngleChange: "); Serial.println(angleChange);

    if (angleChange >= 30 && angleChange <= 400) {
      trigger3 = true;
      trigger2 = false;
      trigger2count = 0;
      Serial.println("TRIGGER 3");
    }
    if (trigger2count >= 6) {
      trigger2 = false;
      trigger2count = 0;
    }
  }

  //TRIGGER 3 (Settle)
  if (trigger3) {
    trigger3count++;
    if (trigger3count >= 10) {
      angleChange = sqrt(gx*gx + gy*gy + gz*gz);
      if (angleChange <= 10) fall = true;

      trigger3 = false;
      trigger3count = 0;
    }
  }

  // FALL
  if (fall) {
    Serial.println("******** FALL DETECTED ********");
    fall = false;
  }

  // CSV Print for Excel
  Serial.print(t); Serial.print(", ");
  Serial.print(Amp); Serial.print(", ");
  Serial.print(trigger1 ? "1" : "0"); Serial.print(", ");
  Serial.print(trigger2 ? "1" : "0"); Serial.print(", ");
  Serial.print(angleChange); Serial.print(", ");
  Serial.print(trigger3 ? "1" : "0"); Serial.print(", ");
  Serial.println(fall ? "1" : "0");

  delay(100);
}

void mpu_read() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  if (Wire.available() == 14) {
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    Tmp = Wire.read() << 8 | Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();
  }
}