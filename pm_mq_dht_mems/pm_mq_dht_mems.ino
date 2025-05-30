#include <WiFi.h>


#include <ThingSpeak.h>
#include <DHT.h>
#include <SoftwareSerial.h>

// WiFi & ThingSpeak Config
#define WIFI_SSID "RVCE_WiFi"
#define WIFI_PASS "your_password"
#define CHANNEL_ID 123456          // Replace with your ThingSpeak channel ID
#define WRITE_API_KEY "YOUR_API_KEY"

// Sensor Pins
#define PM5003_TX 16               // PM5003 UART (RX->GPIO16)
#define PM5003_RX 17               // PM5003 UART (TX->GPIO17)
#define MQ2_PIN 32                 // MQ-2 Analog (GPIO32)
#define MQ7_PIN 33                 // MQ-7 Analog (GPIO33)
#define MQ135_PIN 34               // MQ-135 Analog (GPIO34)
#define MIC_PIN 35                 // MEMS Microphone (GPIO35)
#define DHT_PIN 4                  // DHT11 Digital (GPIO4)

// Multiplexer Config (HC4051)
#define MUX_S0 25                  // MUX control pins
#define MUX_S1 26
#define MUX_S2 27
#define MUX_SIG 34                 // Shared analog pin (MQ-135/MIC)

// DHT Config
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// PM5003 UART
SoftwareSerial pmSerial(PM5003_RX, PM5003_TX);

// Struct for PM5003 data
struct PM5003_DATA {
  uint16_t pm1_0, pm2_5, pm10;
};

// Calibration Values
#define MQ2_R0 9.83                // MQ-2 resistance in clean air
#define MQ7_R0 27.0                // MQ-7 resistance in clean air
#define MQ135_R0 76.63             // MQ-135 resistance in clean air

WiFiClient client;

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  pmSerial.begin(9600);
  dht.begin();
  
  // Initialize multiplexer pins
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  
  ThingSpeak.begin(client);
}

// ================== MAIN LOOP ==================
void loop() {
  // Read all sensors
  PM5003_DATA pmData;
  bool pmSuccess = readPM5003(&pmData);
  float mq2Value = readMQ2();
  float mq7Value = readMQ7();
  float mq135Value = readMQ135();
  float loudness = readLoudness();
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Print to Serial Monitor
  Serial.println("===== Sensor Readings =====");
  if (pmSuccess) {
    Serial.printf("PM1.0: %d μg/m³ | PM2.5: %d μg/m³ | PM10: %d μg/m³\n", 
                  pmData.pm1_0, pmData.pm2_5, pmData.pm10);
  }
  Serial.printf("MQ-2 (LPG): %.2f ppm | MQ-7 (CO): %.2f ppm | MQ-135 (CO₂): %.2f ppm\n", 
                mq2Value, mq7Value, mq135Value);
  Serial.printf("Loudness: %.2f dB | Temp: %.1f°C | Humidity: %.1f%%\n", 
                loudness, temp, humidity);

  // Send to ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    ThingSpeak.setField(1, pmData.pm1_0);
    ThingSpeak.setField(2, pmData.pm2_5);
    ThingSpeak.setField(3, mq2Value);
    ThingSpeak.setField(4, mq7Value);
    ThingSpeak.setField(5, mq135Value);
    ThingSpeak.setField(6, loudness);
    ThingSpeak.setField(7, temp);
    ThingSpeak.setField(8, humidity);
    
    int status = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);
    Serial.println(status == 200 ? "Data sent!" : "Send failed.");
  }

  delay(15000); // Respect ThingSpeak's 15s limit
}

// ================== SENSOR FUNCTIONS ==================

// Read PM5003 (UART)
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

// Read MQ-2 (LPG/Smoke)
float readMQ2() {
  int raw = analogRead(MQ2_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage * 5.0;
  float ratio = rs / MQ2_R0;
  return 1000 * pow(ratio / 1000, -2.95); // LPG ppm
}

// Read MQ-7 (CO)
float readMQ7() {
  int raw = analogRead(MQ7_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage * 5.0;
  float ratio = rs / MQ7_R0;
  return 1000 * pow(ratio / 1000, -1.53); // CO ppm
}

// Read MQ-135 (CO₂/NH₃)
float readMQ135() {
  int raw = analogRead(MQ135_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage * 5.0;
  float ratio = rs / MQ135_R0;
  return 1000 * pow(ratio / 1000, -3.5); // CO₂ ppm (calibrate!)
}

// Read Loudness (Analog)
float readLoudness() {
  int raw = analogRead(MIC_PIN);
  float voltage = raw * (3.3 / 4095.0);
  return 20 * log10(voltage / 0.00631); // dB
}

// HC4051 Multiplexer Control (if used)
void selectMuxChannel(byte channel) {
  digitalWrite(MUX_S0, bitRead(channel, 0));
  digitalWrite(MUX_S1, bitRead(channel, 1));
  digitalWrite(MUX_S2, bitRead(channel, 2));
  delay(10);
}