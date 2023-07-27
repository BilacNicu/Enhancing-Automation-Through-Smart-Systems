#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <string>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <fstream>
#include <sstream>
#include <json.hpp>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Nicu dejun";
const char* password = "brutus1234";
bool ledSensorValue = false;
bool ACLED = false;
bool HeatingLED = false;
static float updatedTemperature = 0;

const int doorSensorPin = 12;

const int triggerPin = 17;     
const int echoPin = 5;         
const int ledPinHall = 16;     

const int analogPinGas = 34;   
const int acLedPin = 4;        
const int heatingLedPin = 2;   

int kitchenLED = 33;

int thresholdValue = 2000;     

void loop();
void setup() {
    Serial.begin(115200);
    dht.begin();

    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    } else {
        Serial.println("SPIFFS began successfully");
    }

    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(ledPinHall, OUTPUT);
    pinMode(acLedPin, OUTPUT);
    pinMode(heatingLedPin, OUTPUT);
    pinMode(kitchenLED,OUTPUT);
    pinMode(doorSensorPin,INPUT_PULLUP);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi.");
}

void loop() {
    digitalWrite(triggerPin, LOW);
    delay(1000);
    digitalWrite(triggerPin, HIGH);
    delay(1000);
    digitalWrite(triggerPin, LOW);
    int doorSensorState = digitalRead(doorSensorPin);
    if(doorSensorState == HIGH)
    {
        Serial.print("\nDoor is Open\n");
    }
    else
    {
        Serial.print("\nDoor is Closed\n");
    }

    long duration = pulseIn(echoPin, HIGH);
    long distance = duration * 0.034 / 2;

    float temperature = dht.readTemperature();

    if (isnan(temperature)) {
        Serial.println("\nFailed to read temperature from DHT sensor");
    } else {
        Serial.print("Temperature: ");
        Serial.println(temperature);
    }

        if (temperature >= updatedTemperature) {
            Serial.println("\nCurrent recorded temperature is: ");
            Serial.println(temperature);
            Serial.println("\nCurrent Theromstat temperature is: ");
            Serial.println(updatedTemperature);
            digitalWrite(acLedPin, HIGH);
            digitalWrite(heatingLedPin, LOW);
            ACLED = true;
            HeatingLED = false;
        } else if(temperature <= updatedTemperature){
            digitalWrite(acLedPin, LOW);
            digitalWrite(heatingLedPin, HIGH);
            HeatingLED = true;
            ACLED = false;
        }
        else{
            Serial.println("Temps are equal");
        }


    int gasAnalogValue = analogRead(analogPinGas);

    if (distance < 5) {
        digitalWrite(ledPinHall, HIGH);
        Serial.print("Hall Light is ON");
        ledSensorValue = true;
        delay(5000);
    } else {
        digitalWrite(ledPinHall, LOW);
        ledSensorValue = false;
        Serial.print("Hall Light is OFF\n");
    }

    DynamicJsonDocument jsonDoc(1024);
    jsonDoc["Home Temperature"] = temperature;
    jsonDoc["Air Conditioning"] = ACLED;
    jsonDoc["Heating"] = HeatingLED;
    jsonDoc["GasAnalogValue"] = gasAnalogValue;
    jsonDoc["Sensor1"] = "Hall Light";
    jsonDoc["Value1"] = ledSensorValue;
    jsonDoc["KitchenLED"] = kitchenLED;
    jsonDoc["Door Sensor"] = doorSensorState;
 
    DynamicJsonDocument jsonDocTemp(1024);
    String jsonStringTemp;
    serializeJson(jsonDocTemp, jsonStringTemp);

    File jsonFileTemp = SPIFFS.open("/spiffs/sensorsTemp.json", "w");
    if (!jsonFileTemp) {
        Serial.println("Failed to open file for reading");
    } else {
        jsonFileTemp.println(jsonStringTemp);
        String jsonDataTemp = jsonFileTemp.readString();
        jsonFileTemp.close();
        Serial.println("\nJson Temp Data: " + jsonDataTemp + "\n");
    }
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX //
    if (gasAnalogValue > thresholdValue) {
        jsonDoc["GasSafety"] = "Warning! Unsafe Gas Levels or Smoke Detected!";
    } else {
        jsonDoc["GasSafety"] = "Safe";
    }

    String jsonString;
    serializeJson(jsonDoc, jsonString);

    File fileToWrite = SPIFFS.open("/spiffs/sensors.json", "w");
    if (!fileToWrite) {
        Serial.println("Failed to open file for writing");
    } else {
        fileToWrite.println(jsonString);
        fileToWrite.close();
    }
    

    delay(500);

    File jsonFile = SPIFFS.open("/spiffs/sensors.json", "r");
    if (!jsonFile) {
        Serial.println("Failed to open file for reading");
    } else {
        String jsonData = jsonFile.readString();
        jsonFile.close();
        Serial.println("\n" + jsonData + "\n");
    }

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient client2;
        client2.begin("http://192.168.234.50:5000");
        client2.addHeader("Content-Type", "application/json");

        int httpResponseCode = client2.PUT(jsonStringTemp);

  if (httpResponseCode == 200) {
            // Successfully received the updated JSON data
            // Parse the response JSON to get the updated data
            String response = client2.getString();
            DeserializationError error = deserializeJson(jsonDocTemp, response);
            if (error) {
                Serial.println("Failed to parse response JSON");
            } else {
                // Get the updated temperature value from the response
                updatedTemperature = jsonDocTemp["Thermostat Temperature"];
                // digitalWrite(kitchenLED, jsonDocTemp["KitchenLED"]);
                // Do something with the updated temperature value
                Serial.print("Updated temperature: ");
                Serial.println(updatedTemperature);
            }
        } else {
            Serial.print("PUT request failed. Error code: ");
            Serial.println(httpResponseCode);
        }
  client2.end();

        HTTPClient client;
        client.begin("http://192.168.234.50:5000");
        client.addHeader("Content-Type", "application/json");

        int httpCode = client.POST(jsonString);

        if (httpCode > 0) {
            String payload = client.getString();
            Serial.println("\nStatus code: " + String(httpCode));
            Serial.println(payload);
        } else {
            Serial.println("Error on HTTP request: " + String(httpCode));
        }
    } else {
        Serial.println("Connection lost");
    }

    delay(1000);
}