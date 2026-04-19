#include <Wire.h>
#include <MAX30205.h>
#include <MPU6050.h>
#include <MAX30102.h>
#include <spo2_algorithm.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Config
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPass";
const char* ROSbot_IP = "192.168.1.100";
const int ROSbot_PORT = 5005;
WiFiUDP udp;

MAX30205 tempSensor;
MPU6050 mpu;
MAX30102 particleSensor;

// HR & SpO2 Buffers
uint32_t irBuffer[100]; 
uint32_t redBuffer[100];
int32_t bufferLength = 100; 
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// Fall Triggers 
float Amp = 0;
bool trigger1 = false, trigger2 = false, trigger3 = false;
byte trigger1count = 0, trigger2count = 0, trigger3count = 0;
bool fall = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(21,22); 
  Serial.println("=== Smart Bio-Wearable with Wi-Fi Alerts ===");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  tempSensor.begin();
  mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) Serial.println("MAX30102 NOT FOUND");
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
}

void loop() {
  // 1. COLLECT SAMPLES FOR HR/SPO2
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // 2. CALCULATE HR & SPO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // 3. READ TEMP
  float tempValue = tempSensor.readTemperature();

  // 4. FALL DETECTION LOGIC
  Vector normAccel = mpu.readNormalizeAccel();
  Vector normGyro = mpu.readNormalizeGyro();
  Amp = sqrt(pow(normAccel.XAxis,2) + pow(normAccel.YAxis,2) + pow(normAccel.ZAxis,2)) / 9.8 * 10;

  if (Amp <= 2.0 && !trigger1) { trigger1 = true; trigger1count = 0; }
  if (trigger1) {
    trigger1count++;
    if (Amp >= 12.0) { trigger2 = true; trigger1 = false; trigger2count = 0; }
    if (trigger1count > 6) trigger1 = false;
  }
  if (trigger2) {
    trigger2count++;
    float angleChange = sqrt(pow(normGyro.XAxis,2) + pow(normGyro.YAxis,2) + pow(normGyro.ZAxis,2));
    if (angleChange >= 30) { trigger3 = true; trigger2 = false; trigger3count = 0; }
    if (trigger2count > 6) trigger2 = false;
  }
  if (trigger3) {
    trigger3count++;
    if (trigger3count > 10) { fall = true; trigger3 = false; }
  }

  // 5. OUTPUT & ALERTS
  if (validHeartRate && validSPO2) {
    Serial.print("HR: "); Serial.print(heartRate);
    Serial.print(" SpO2: "); Serial.println(spo2);
  }

  if (fall) {
    udp.beginPacket(ROSbot_IP, ROSbot_PORT);
    udp.print("ALERT:FALL");
    udp.endPacket();
    Serial.println("🚨 FALL!");
    fall = false;
  }

  if (tempValue > 38.0) {
    udp.beginPacket(ROSbot_IP, ROSbot_PORT);
    udp.print("ALERT:FEVER");
    udp.endPacket();
  }

  delay(100);
}