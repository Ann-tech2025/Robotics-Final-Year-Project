#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"  // SparkFun/MAX3010x SpO2 algorithm

MAX30105 particleSensor;

#define SDA_PIN 26
#define SCL_PIN 25
#define BUFFER_LENGTH 100
#define IR_THRESHOLD 20000  // Minimum IR for finger detection

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);  // ESP32 custom I2C pins

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 not found. Check wiring/power.");
    while (1);
  }

  // Configure sensor: LED brightness, sample averaging, mode, sample rate, pulse width, ADC range
  particleSensor.setup(60, 4, 2, 100, 411, 4096);  // Red + IR, moderate settings

  Serial.println("Place your finger on the sensor...");
  delay(10);  // Let sensor stabilize
}

void loop() {

  // Collect 100 samples
  for (byte i = 0; i < BUFFER_LENGTH; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
      delay(1);  // Prevent ESP32 watchdog reset
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Finger detection
  if (irBuffer[BUFFER_LENGTH - 1] < IR_THRESHOLD) {
    Serial.println("No finger detected | HR=0 | SPO2=0");
    delay(1000);
    return;  // Skip calculations
  }

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, BUFFER_LENGTH,
    redBuffer,
    &spo2, &validSPO2,
    &heartRate, &validHeartRate
  );

  // Output results
  Serial.print("HR=");
  Serial.print(validHeartRate ? heartRate : 0);
  Serial.print(" | SPO2=");
  Serial.println(validSPO2 ? spo2 : 0);

  delay(500);  // Slow down loop to prevent flooding serial
}