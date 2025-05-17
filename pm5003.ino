#include <WiFi.h>
#include <ThingSpeak.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "RVCE_WiFi";
const char* password = "your_password";

// ThingSpeak settings
unsigned long channelID = YOUR_CHANNEL_ID;
const char* writeAPIKey = "YOUR_WRITE_API_KEY";

// PM5003 sensor pins
#define PM_TX 16  // PM5003 TX -> ESP32 RX (GPIO16)
#define PM_RX 17  // PM5003 RX -> ESP32 TX (GPIO17)

WiFiClient client;
SoftwareSerial pmSerial(PM_RX, PM_TX);  // RX, TX

struct PM5003_DATA {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10;
};

void setup() {
  Serial.begin(115200);
  pmSerial.begin(9600);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  
  ThingSpeak.begin(client);
}

bool readPM5003(PM5003_DATA* data) {
  uint8_t buffer[32];
  int index = 0;
  
  while (pmSerial.available()) {
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
    
    if (++index >= 32) {
      // Verify checksum
      uint16_t checksum = 0;
      for (int i = 0; i < 30; i++) checksum += buffer[i];
      
      if (checksum == (buffer[30] << 8 | buffer[31])) {
        data->pm1_0 = buffer[10] << 8 | buffer[11];
        data->pm2_5 = buffer[12] << 8 | buffer[13];
        data->pm10 = buffer[14] << 8 | buffer[15];
        return true;
      }
      index = 0;
    }
  }
  return false;
}

void loop() {
  PM5003_DATA pmData;
  
  if (readPM5003(&pmData)) {
    Serial.print("PM1.0: "); Serial.print(pmData.pm1_0); Serial.println(" μg/m³");
    Serial.print("PM2.5: "); Serial.print(pmData.pm2_5); Serial.println(" μg/m³");
    Serial.print("PM10: "); Serial.print(pmData.pm10); Serial.println(" μg/m³");
    
    // Send to ThingSpeak
    ThingSpeak.setField(1, pmData.pm1_0);  // Field 1: PM1.0
    ThingSpeak.setField(2, pmData.pm2_5);  // Field 2: PM2.5
    ThingSpeak.setField(3, pmData.pm10);   // Field 3: PM10
    
    int status = ThingSpeak.writeFields(channelID, writeAPIKey);
    
    if (status == 200) {
      Serial.println("Data sent to ThingSpeak!");
    } else {
      Serial.println("Error sending data. HTTP code: " + String(status));
    }
  } else {
    Serial.println("Failed to read PM5003 data");
  }
  
  delay(15000);  // Respect ThingSpeak's 15s limit
}
