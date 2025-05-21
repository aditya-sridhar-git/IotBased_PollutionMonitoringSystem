/*
 * IoT-Based Pollution Monitoring System
 * 
 * This integrated system monitors:
 * - Sound levels (loudness sensor)
 * - Air quality (MQ-2 for LPG/Smoke, MQ-7 for CO)
 * - Particulate matter (PM5003 for PM1.0, PM2.5, PM10)
 * 
 * Data is sent to ThingSpeak for storage and analysis
 */

#include <WiFi.h>
#include <ThingSpeak.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "";  // Your WiFi SSID
const char* password = "";  // Your WiFi password

// ThingSpeak settings
unsigned long channelID = 0;  // Your ThingSpeak channel ID
const char* writeAPIKey = "";  // Your ThingSpeak Write API Key

// Sensor pins
#define LOUDNESS_PIN 32  // Analog pin for loudness sensor
#define MQ2_PIN 33       // Analog pin for MQ-2 (LPG, smoke)
#define MQ7_PIN 35       // Analog pin for MQ-7 (CO)
#define PM_TX 16         // PM5003 TX -> ESP32 RX
#define PM_RX 17         // PM5003 RX -> ESP32 TX

// Calibration values for gas sensors
#define MQ2_R0 9.83      // MQ-2 Sensor resistance in clean air
#define MQ7_R0 27.0      // MQ-7 Sensor resistance in clean air

// Create software serial for PM5003
SoftwareSerial pmSerial(PM_RX, PM_TX);  // RX, TX

// PM5003 data structure
struct PM5003_DATA {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10;
};

WiFiClient client;

void setup() {
  Serial.begin(115200);
  pmSerial.begin(9600);  // Initialize PM5003 serial
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nConnection failed. Restarting ESP32");
    ESP.restart();
  }
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  Serial.println("Pollution monitoring system initialized");
}

// Function to read loudness sensor
float readLoudness() {
  int rawValue = analogRead(LOUDNESS_PIN);
  
  // Convert to voltage (ESP32: 0-3.3V, 12-bit ADC)
  float voltage = rawValue * (3.3 / 4095.0);
  
  // Optional: Convert to dB (approximate, requires calibration)
  // float dB = 20 * log10(voltage / 0.00631);  // Example formula
  
  return rawValue;  // Return raw ADC value (or voltage/dB if calibrated)
}

// Function to read MQ-2 sensor (LPG/Smoke)
float readMQ2() {
  int rawValue = analogRead(MQ2_PIN);
  float voltage = rawValue * (3.3 / 4095.0);  // ESP32 ADC resolution
  float rs = (3.3 - voltage) / voltage * 5.0;  // Calculate sensor resistance
  float ratio = rs / MQ2_R0;
  
  // Approximate ppm calculation for LPG (simplified)
  float ppm_lpg = 1000 * pow(ratio / 1000, -2.95);
  return ppm_lpg;
}

// Function to read MQ-7 sensor (CO)
float readMQ7() {
  int rawValue = analogRead(MQ7_PIN);
  float voltage = rawValue * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage * 5.0;
  float ratio = rs / MQ7_R0;
  
  // Approximate ppm calculation for CO (simplified)
  float ppm_co = 1000 * pow(ratio / 1000, -1.53);
  return ppm_co;
}

// Function to read PM5003 sensor
bool readPM5003(PM5003_DATA* data) {
  uint8_t buffer[32];
  int index = 0;
  unsigned long timeout = millis() + 1000;  // 1 second timeout
  
  // Clear any old data
  while (pmSerial.available()) {
    pmSerial.read();
  }
  
  // Wait for data
  while (index < 32) {
    if (pmSerial.available()) {
      buffer[index] = pmSerial.read();
      
      // Check packet header (0x42, 0x4D)
      if (index == 0 && buffer[0] != 0x42) {
        index = 0;
        continue;
      }
      if (index == 1 && buffer[1] != 0x4D) {
        index = 0;
        continue;
      }
      
      index++;
      
      if (index == 32) {
        // Verify checksum
        uint16_t checksum = 0;
        for (int i = 0; i < 30; i++) {
          checksum += buffer[i];
        }
        
        if (checksum == ((buffer[30] << 8) | buffer[31])) {
          data->pm1_0 = (buffer[10] << 8) | buffer[11];
          data->pm2_5 = (buffer[12] << 8) | buffer[13];
          data->pm10 = (buffer[14] << 8) | buffer[15];
          return true;
        }
        index = 0;  // Reset on checksum fail
      }
    }
    
    if (millis() > timeout) {
      return false;  // Timeout
    }
  }
  return false;
}

void loop() {
  // Read all sensor values
  float loudnessValue = readLoudness();
  float mq2Value = readMQ2();
  float mq7Value = readMQ7();
  
  // PM5003 data
  PM5003_DATA pmData = {0, 0, 0};  // Initialize with zeros
  bool pmReadSuccess = readPM5003(&pmData);
  
  // Print sensor readings to serial monitor
  Serial.println("=========== SENSOR READINGS ===========");
  Serial.print("Loudness: "); Serial.println(loudnessValue);
  Serial.print("MQ-2 (LPG/Smoke): "); Serial.print(mq2Value); Serial.println(" ppm");
  Serial.print("MQ-7 (CO): "); Serial.print(mq7Value); Serial.println(" ppm");
  
  if (pmReadSuccess) {
    Serial.print("PM1.0: "); Serial.print(pmData.pm1_0); Serial.println(" μg/m³");
    Serial.print("PM2.5: "); Serial.print(pmData.pm2_5); Serial.println(" μg/m³");
    Serial.print("PM10: "); Serial.print(pmData.pm10); Serial.println(" μg/m³");
  } else {
    Serial.println("Failed to read PM5003 data");
  }
  
  // Set ThingSpeak fields
  ThingSpeak.setField(1, loudnessValue);  // Field 1: Loudness
  ThingSpeak.setField(2, mq2Value);       // Field 2: MQ-2 (LPG/Smoke)
  ThingSpeak.setField(3, mq7Value);       // Field 3: MQ-7 (CO)
  
  if (pmReadSuccess) {
    ThingSpeak.setField(4, pmData.pm1_0);  // Field 4: PM1.0
    ThingSpeak.setField(5, pmData.pm2_5);  // Field 5: PM2.5
    ThingSpeak.setField(6, pmData.pm10);   // Field 6: PM10
  }
  
  // Send to ThingSpeak
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  
  if (status == 200) {
    Serial.println("Data sent to ThingSpeak successfully!");
  } else {
    Serial.println("Problem sending data. HTTP error code: " + String(status));
  }
  
  Serial.println("========================================");
  Serial.println("Waiting for next measurement cycle...");
  
  // ThingSpeak free account requires 15s between updates
  delay(15000);
}