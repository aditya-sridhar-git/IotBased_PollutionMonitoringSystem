#include <Wifi.h>
#include <ThingSpeak.h>

#define LOUDNESS_PIN = 32

const char* ssid = ""; //Wifi name
const char* password = ""; //Password

unsigned long channelID = ""; //ThingSpeak Channel ID
const char* writeAPIKey = ""; // ThingSpeal writeAPIKey

int fieldNumber;

WiFiClient client;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid,password);

  status = WiFi.status();
  int timeout;

  while (status!=3 && timeout <20){   // WL_CONNECTED = 3 (for successfull connection)
    Serial.println("Connecting...");
    timeout++;
  }

  if (status==3){
    
    Serial.println("Connected Successfully");
    Serial.println("IP configuration of the network is "+ WiFi.localIP());
  }

  else{
    Serial.println("Connection was unsuccessfull");
    Serial.println(status);
    Serial.println("Restarting ESP32");
    ESP.restart();

  }

  ThingSpeak.begin(client);




}

float readLoudness(){
  
  int rawValue = analogRead(LOUDNESS_SENSOR_PIN);
  
  // Convert to voltage (ESP32: 0-3.3V, 12-bit ADC)
  float voltage = rawValue * (3.3 / 4095.0);  // For ESP32 (adjust for Arduino)
  
  // Optional: Convert to dB (approximate, requires calibration)
  // float dB = 20 * log10(voltage / 0.00631);  // Example formula
  
  return rawValue;  // Return raw ADC value (or voltage/dB if calibrated)

}

void loop() {
  // put your main code here, to run repeatedly:
  int rawValue = analogRead(LOUDNESS_PIN);
  ThingSpeak.setField(fieldNumber,readLoudness());


  int status = ThingSpeak.writeFields(channelID,writeAPIKey);

  if (status==200){
    Serial.println("Data successfully sent to ThingSpeak");
  }
  else{
    Serial.println("Data transfer failed, try again");
    Serial.printn(String{status));
  }

    delay(15000); // free version of ThinSpeak allows for only https requests to be sent for an interval of 15s 
    


}
