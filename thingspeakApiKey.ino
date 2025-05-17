#include <WiFi.h>
#include "ThingSpeak.h"

const char* ssid = "";           // Your Wi-Fi name
const char* password = "";   // Your Wi-Fi password

WiFiClient client;

unsigned long myChannelNumber = 2965771;        // Replace with your ThingSpeak channel number
const char* myWriteAPIKey = "I6D9WGROEFOLZJIE";     // Replace with your ThingSpeak Write API Key

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected!");
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {
  int airValue = 200;      // Example pin
  int soundValue = 100;    // Example pin

  ThingSpeak.setField(1, airValue);   // Send to Field 1
  ThingSpeak.setField(2, soundValue); // Send to Field 2

  int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (status == 200) {
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Error sending data. HTTP error code: " + String(status));
  }

  delay(15000);  // Wait 15 seconds (ThingSpeak rule: max 1 update every 15s)
}