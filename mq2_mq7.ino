#include <WiFi.h>
#include <ThingSpeak.h>

// WiFi credentials
const char* ssid = "RVCE_WiFi";
const char* password = "your_password";

// ThingSpeak settings
unsigned long channelID = YOUR_CHANNEL_ID; // Replace with your channel ID
const char* writeAPIKey = "YOUR_WRITE_API_KEY"; // Replace with your write API key

// Sensor pins
#define MQ2_PIN 33  // Analog pin for MQ-2
#define MQ7_PIN 35  // Analog pin for MQ-7

// Calibration values
#define MQ2_R0 9.83  // MQ-2 Sensor resistance in clean air
#define MQ7_R0 27.0  // MQ-7 Sensor resistance in clean air

WiFiClient client;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  
  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

float readMQ2() {
  int rawValue = analogRead(MQ2_PIN);
  float voltage = rawValue * (3.3 / 4095.0); // ESP32 ADC resolution
  float rs = (3.3 - voltage) / voltage * 5.0; // Calculate sensor resistance
  float ratio = rs / MQ2_R0;
  
  // Approximate ppm calculation for LPG (simplified)
  float ppm_lpg = 1000 * pow(ratio / 1000, -2.95); 
  return ppm_lpg;
}

float readMQ7() {
  int rawValue = analogRead(MQ7_PIN);
  float voltage = rawValue * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage * 5.0;
  float ratio = rs / MQ7_R0;
  
  // Approximate ppm calculation for CO (simplified)
  float ppm_co = 1000 * pow(ratio / 1000, -1.53);
  return ppm_co;
}

void loop() {
  // Read sensor values
  float mq2Value = readMQ2();  // Get MQ-2 reading (LPG/Smoke in ppm)
  float mq7Value = readMQ7();  // Get MQ-7 reading (CO in ppm)
  
  // Print to serial monitor
  Serial.print("MQ-2 (LPG): "); Serial.print(mq2Value); Serial.println(" ppm");
  Serial.print("MQ-7 (CO): "); Serial.print(mq7Value); Serial.println(" ppm");
  
  // Set ThingSpeak fields
  ThingSpeak.setField(1, mq2Value); // Field 1 for MQ-2
  ThingSpeak.setField(2, mq7Value); // Field 2 for MQ-7
  
  // Send to ThingSpeak
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  
  if(status == 200) {
    Serial.println("Data sent to ThingSpeak successfully!");
  } else {
    Serial.println("Problem sending data. HTTP error code: " + String(status));
  }
  
  delay(15000); // ThingSpeak free account requires 15s between updates
}
