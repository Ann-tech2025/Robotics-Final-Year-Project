#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "DHT.h"

// Initialize I2C
#define SDA_PIN 21
#define SCL_PIN 22

// Initialize MAX30102
MAX30105 particleSensor;

uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
int bufferIndex = 0;
unsigned long lastCompute = 0;
// Initialize DHT 
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float lastTemp = 0;
unsigned long tempTimer = 0;

//MPU6050 
#define MPU_addr 0x68

int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

//FALL
bool fall = false;
bool trigger1 = false;
bool trigger2 = false;
bool trigger3 = false;

byte trigger1count = 0;
byte trigger2count = 0;
byte trigger3count = 0;

float angleChange = 0;

// SETUP
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 NOT FOUND");
    while (1);
  }

  particleSensor.setup(60, 4, 2, 100, 411, 4096);

  // DHT
  dht.begin();

  // MPU6050 wake
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("REAL-TIME SYSTEM STARTED 🚀");
}

// MPU READ
void mpu_read() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  if (Wire.available() == 14) {
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    Wire.read(); Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();
  }
}

void loop() {

  // REAL-TIME MAX30102 STREAM
  if (particleSensor.available()) {
    redBuffer[bufferIndex] = particleSensor.getRed();
    irBuffer[bufferIndex] = particleSensor.getIR();
    particleSensor.nextSample();
    bufferIndex++;
    if (bufferIndex >= 100) bufferIndex = 0;
  }
  if (millis() - lastCompute >= 1000) {   //Compute every 1 second
    lastCompute = millis();

    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, 100,
      redBuffer,
      &spo2, &validSPO2,
      &heartRate, &validHeartRate
    );
  }
//CLEAN HR + SPO2 
  int finalHR = validHeartRate ? heartRate : 0;
  int finalSpO2 = validSPO2 ? spo2 : 0;
  if (finalHR < 40 || finalHR > 180) finalHR = 0;
  if (finalSpO2 < 70 || finalSpO2 > 100) finalSpO2 = 0;

  // TEMPERATURE 
  if (millis() - tempTimer > 2000) {
    tempTimer = millis();

    float t = dht.readTemperature();
    if (!isnan(t)) lastTemp = t;
  }

  if (lastTemp > 38.0) {
    Serial.println("🔥 HIGH TEMPERATURE ALERT!");
  }

  // MPU FALL
  mpu_read();

  float ax = AcX / 16384.0;
  float ay = AcY / 16384.0;
  float az = AcZ / 16384.0;

  float Amp = sqrt(ax * ax + ay * ay + az * az) * 10;

  // STAGE 1 
  if (Amp <= 2.0 && !trigger1 && !trigger2) {
    trigger1 = true;
    trigger1count = 0;
  }

  if (trigger1) {
    trigger1count++;

    if (Amp >= 12.0) {
      trigger2 = true;
      trigger1 = false;
      trigger2count = 0;
    }

    if (trigger1count > 6) trigger1 = false;
  }

  // STAGE 2
  if (trigger2) {
    trigger2count++;

    angleChange = sqrt(GyX*GyX + GyY*GyY + GyZ*GyZ);

    if (angleChange >= 30 && angleChange <= 400) {
      trigger3 = true;
      trigger2 = false;
      trigger3count = 0;
    }

    if (trigger2count > 6) trigger2 = false;
  }

  // STAGE 3
  if (trigger3) {
    trigger3count++;

    if (trigger3count > 10) {
      float finalAngle = sqrt(GyX*GyX + GyY*GyY + GyZ*GyZ);

      if (finalAngle < 10) fall = true;

      trigger3 = false;
    }
  }

  if (fall) {
    Serial.println("🚨 FALL DETECTED!");
    fall = false;
  }

  // HR ALERT
  static int hrCount = 0;

  if (finalHR > 150) {
    hrCount++;
  } else {
    hrCount = 0;
  }

  if (hrCount >= 3) {
    Serial.println("🚨 HIGH HEART RATE ALERT!");
  }

  // OUTPUT
  Serial.print("HR=");
  Serial.print(finalHR);

  Serial.print(" | SPO2=");
  Serial.print(finalSpO2);

  Serial.print(" | Temp=");
  Serial.print(lastTemp);

  Serial.print(" | Motion=");
  Serial.print(Amp);

  Serial.println();

  delay(50); // fast + real-time feel
}