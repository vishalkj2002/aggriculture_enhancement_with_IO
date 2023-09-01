/*
  WriteMultipleFields
  
  Description: Writes values to fields 1,2,3,4 and status in a single ThingSpeak update every 20 seconds.
  
  Hardware: ESP32 based boards
  
  !!! IMPORTANT - Modify the secrets.h file for this project with your network connection and ThingSpeak channel details. !!!
  
  Note:
  - Requires installation of EPS32 core. See https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md for details. 
  - Select the target hardware from the Tools->Board menu
  - This example is written for a network using WPA encryption. For WEP or WPA, change the WiFi.begin() call accordingly.
  
  ThingSpeak ( https://www.thingspeak.com ) is an analytic IoT platform service that allows you to aggregate, visualize, and 
  analyze live data streams in the cloud. Visit https://www.thingspeak.com to sign up for a free account and create a channel.  
  
  Documentation for the ThingSpeak Communication Library for Arduino is in the README.md folder where the library was installed.
  See https://www.mathworks.com/help/thingspeak/index.html for the full ThingSpeak documentation.
  
  For licensing information, see the accompanying license file.
  
  Copyright 2020, The MathWorks, Inc.
*/

#include <Wire.h>
#include <WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

// I2C address of the sunlight sensor
const int sunlightSensorAddress = 0x53;

// Pin definitions
const int moistureSensorPin = 4;   // Connect non-corrosive soil moisture sensor to GPIO33_ADC
const int ldrDigitalPin = 8;       // Digital pin for LDR module
const int relayControlPin = 12;    // Digital pin for relay control (choose any available GPIO pin)

const int moistureThreshold = 50; // Adjust this threshold as needed

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

void setup() {
  pinMode(ldrDigitalPin, INPUT); // Set LDR pin as input
  pinMode(relayControlPin, OUTPUT); // Set relay control pin as output

  digitalWrite(relayControlPin, LOW); // Turn off the relay initially

  Wire.begin();
  Serial.begin(115200);  //Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

  int moistureValue = analogRead(moistureSensorPin);
  int ldrValue = digitalRead(ldrDigitalPin); // Read digital LDR pin

  // Read sunlight percentage from I2C sunlight sensor
  Wire.beginTransmission(sunlightSensorAddress);
  Wire.write(0x02); // Register address for sunlight percentage
  Wire.endTransmission(false);
  Wire.requestFrom(sunlightSensorAddress, 2);

  int sunlightValue = Wire.read() << 8 | Wire.read();
  float sunlightPercentage = (sunlightValue / 65535.0) * 100.0;

  // Convert analog readings to meaningful values
  int moisturePercentage = map(moistureValue, 0, 4095, 0, 100);

  // Determine LDR status based on digital value
  String ldrStatus = (ldrValue == HIGH) ? "Dark" : "Bright";

  // Control the relay (and pump) based on moisture level
  if (moisturePercentage > moistureThreshold) {
    digitalWrite(relayControlPin, HIGH); // Turn on the relay (and pump)
  } else {
    digitalWrite(relayControlPin, LOW); // Turn off the relay (and pump)
  }

  // Print the sensor data to the Serial Monitor
  Serial.println("Sensor Data:");
  Serial.print("Sunlight Percentage: ");
  Serial.print(sunlightPercentage);
  Serial.println("%");
  Serial.print("Moisture Percentage: ");
  Serial.print(moisturePercentage);
  Serial.println("%");
  Serial.print("LDR Status: ");
  Serial.println(ldrStatus);

  delay(5000); // Wait for a few seconds before the next reading

  // set the fields with the values
  ThingSpeak.setField(1, sunlightPercentage);
  ThingSpeak.setField(2, moisturePercentage);
  ThingSpeak.setField(3, (ldrValue == HIGH) ? 0 : 1);
  
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  delay(5000); // Wait for a few seconds before the next reading
}
