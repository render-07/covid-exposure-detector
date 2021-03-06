//////////DEVICE 1////////////
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h> // // For a connection via I2C using the Arduino Wire. Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h"   // legacy: #include "SH1106.h"

#define SERVICE_UUID        "37565ec6-46c8-11ec-81d3-0242ac130003"
#define CHARACTERISTIC_UUID "447cd81e-46c8-11ec-81d3-0242ac130003"
#define RXD2 16
#define TXD2 17

SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL
BLEScan* pBLEScan;
HardwareSerial SerialGPS(1);

TinyGPSPlus gps;
StaticJsonDocument<200> doc;

const char* ssid = "wifiName";
const char* password = "wifiPassword";
const String serverName = "localhost:port/api/information";
const String serverNameGetDevice = "localhost:port/api/devices";
const String locationRoute = "localhost:port/api/location";
const int LED_WIFI = 5; // wifi led indicator
const int LED_POST_DATA = 2; // post data led indicator
const int BUZZER_NEAR_PERSON = 19; // near person indicator
const String DEVICE_NAME = "DEVICE 1";
int exposureCount = 0;
int devicesFound = 0;
String deviceName = "";
float distance = 0;
int scanTime = 5; //In seconds
int8_t N = 2; // index
int8_t A = -69; // // BLE uses Measured Power is also known as the 1 Meter RSSI. So consider the value of Measured Power = -69, indicates the signal strength is received from a reference node, normally at a distance of 1m.
float latitude , longitude;
String latitude_string = "14.5013040- -"; // dummy data
String longitude_string = "121.058395- -"; // dummy data
unsigned long lastTime = 0; // Variables are unsigned longs because the time, measured in milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long timerDelay = 5; // Set timer to 5 seconds (5000)
int capacitySize = 10;

void refreshDisplay(String text) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, text);
  // write the buffer to the display
  display.display();
}

void displayDeviceFound(String text) {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, text);
  // write the buffer to the display
  display.display();
}

void displayDeviceLocation(String text) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, text);
  // write the buffer to the display
  display.display();
}

void displayWelcomeScreen(String text) {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, text);
  // write the buffer to the display
  display.display();
}

String getServiceUUID (String uuid) {
  String serviceUUID = uuid.substring(uuid.indexOf("serviceUUID: "), 93); // get only serviceUUID (92th is the placement of the last char of serviceUUID)
  serviceUUID.replace("serviceUUID: ", ""); // remove the string "serviceUUID"
  return serviceUUID;
}

void postData(String uuid) {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // CHANGE TO NEEDED FIELDS
      http.addHeader("Content-Type", "application/json");

      String serviceUUID = getServiceUUID(uuid);
      doc["fromWhatDevice"] = DEVICE_NAME;
      doc["closeContact"] = "9ecc3b20-46c8-11ec-81d3-0242ac130003"; // MAY SAPAK DEVICE 2
      String json;
      serializeJson(doc, json);
      Serial.println("serviceUUID: " + serviceUUID);

      int httpResponseCode = http.POST(json);

      if (httpResponseCode == 200) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        Serial.println("INSERTED TO DATABASE");
        digitalWrite (LED_POST_DATA, HIGH);
        delay(500);
        digitalWrite (LED_POST_DATA, LOW);
      }
      else {
        refreshDisplay("No connection to the server");
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

bool checkInDatabase (String uuid) {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      String serviceUUID = uuid.substring(uuid.indexOf("serviceUUID: "), 93); // get only serviceUUID (93th is the placement of the last char of serviceUUID)
      serviceUUID.replace("serviceUUID: ", ""); // remove the string "serviceUUID"

      http.begin(client, serverNameGetDevice + "/" + serviceUUID);

      // Serial.println(serverNameGetDevice + "/" + serviceUUID);

      int httpResponseCode = http.GET();
      String payload = "{}";
      payload = http.getString();

      if (httpResponseCode == 200) {
        if (payload != "null") { // if not null
          return true;
        } else { // else payload is "null"
          Serial.println("ID not existing in DB");
          return false;
        }
      }
      else {
        refreshDisplay("No connection to the server");
        return false;
      }

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
      return false;
    }
    lastTime = millis();
  }
}

void sendGPSLocationToDB (String latitude, String longitude) {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(client, locationRoute);

      // CHANGE TO NEEDED FIELDS
      http.addHeader("Content-Type", "application/json");

      doc["uid"] = SERVICE_UUID;
      doc["latitude"] = latitude;
      doc["longitude"] = longitude;
      String json;
      serializeJson(doc, json);

      int httpResponseCode = http.POST(json);

      if (httpResponseCode == 200) {
        // running on background so no display
      }
      else {
        refreshDisplay("No connection to the server");
      }

      // Free resources
      http.end();
    }
    else {
      refreshDisplay("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

String getLocationOfOtherDevice (String uuid) {
  if (uuid != "") {
    //Send an HTTP POST request every 10 minutes
    if ((millis() - lastTime) > timerDelay) {
      //Check WiFi connection status
      if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        // Your Domain name with URL path or IP address with path
        String serviceUUID = uuid.substring(uuid.indexOf("serviceUUID: "), 93); // get only serviceUUID (93th is the placement of the last char of serviceUUID)
        serviceUUID.replace("serviceUUID: ", ""); // remove the string "serviceUUID"

        http.begin(client, locationRoute + "/" + serviceUUID);

        int httpResponseCode = http.GET();
        String payload = "{}";
        payload = http.getString();

        if (httpResponseCode == 200) {
          if (payload != "null") { // if not null
            return payload;
          } else { // else payload is "null"
            return "No updated location of this device yet";
          }
        }
        else {
          return "No connection to the server";
        }

        // Free resources
        http.end();
      }
      else {
        return "WiFi Disconnected";
      }
      lastTime = millis();
    } else {
      return "WiFi Disconnected";
    }

  } else {
    return "Unknown device";
  }
}

boolean checkForDevice(String array[], String element) {
  for (int i = 0; i < capacitySize; i++) {
    if (array[i] == element) {
      return true;
    }
  }
  return false;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2); //gps baud
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_POST_DATA, OUTPUT);
  pinMode(BUZZER_NEAR_PERSON, OUTPUT);

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();

  Serial.println("********************************");
  Serial.println("GROUP 4607");
  displayWelcomeScreen("GROUP 4607");
  Serial.println("********************************");
  delay(5000);

  Serial.println(DEVICE_NAME);
  displayWelcomeScreen(DEVICE_NAME);
  delay(2000);

  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" with password ");
  Serial.println(password);

  refreshDisplay("Connecting to " + String(ssid));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite (LED_WIFI, HIGH);
    delay(100);
    digitalWrite (LED_WIFI, LOW);
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  refreshDisplay("Connected to WiFi network with IP Address: " + String(WiFi.localIP().toString().c_str()));

  // Server part
  BLEDevice::init("DEVICE 1");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("From DEVICE 1");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  // Scanning part
  Serial.println("Scanning...");
  // BLEDevice::init("DEVICE 1");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  // Serial.println(foundDevices.getCount());

  if (foundDevices.getCount() == 0) {
    exposureCount = 0;
  }

  String scannedDevices[capacitySize];
  float rssiOfOtherDevice = 0;

  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    scannedDevices[i] = device.getName().c_str();
    if (strcmp(device.getName().c_str(), "DEVICE 2") == 0) {
      float rssi = (int)device.getRSSI() + 20;
      float ratio = (A - rssi);
      float ratioOver20 = ratio / (20);
      rssiOfOtherDevice = pow(10, ratioOver20); // in meters
    }
  }

  bool andyan = checkForDevice(scannedDevices, "DEVICE 2");

  if (andyan && rssiOfOtherDevice <= 1) {
    exposureCount++;
    for (int i = 0; i < foundDevices.getCount(); i++) {
      BLEAdvertisedDevice device = foundDevices.getDevice(i);
      if (strcmp(device.getName().c_str(), "DEVICE 2") == 0) {
        devicesFound++;
        deviceName = device.toString().c_str();
        Serial.printf("Rssi: %d \n", (int)device.getRSSI());
        float rssi = (int)device.getRSSI() + 20;
        float ratio = (A - rssi);
        float ratioOver20 = ratio / (20);
        // Serial.println(String(rssi));
        // Serial.println("ratio" + String(ratio));
        // Serial.println("ratioOver20" + Sttring(ratioOver20));
        distance = pow(10, ratioOver20); // in meters
        Serial.printf("Rssi-distance in meters: %f", (distance));
        Serial.printf(" m \n");

        Serial.print("Exposure count: " );
        Serial.println(exposureCount);

        // Exposure count of 12 indicates 1 minute (180 == 15 minute)
        if (exposureCount == 6) {
          digitalWrite(BUZZER_NEAR_PERSON, HIGH);
          digitalWrite (LED_WIFI, HIGH);
          delay(3000);
          digitalWrite(BUZZER_NEAR_PERSON, LOW);
          digitalWrite (LED_WIFI, LOW);
          postData(device.toString().c_str());
          exposureCount = 0;
        }
      }
    }
  } else {
    exposureCount = 0;
  }

  Serial.print("Devices found: ");
  Serial.println(devicesFound);
  displayDeviceFound("Devices found: " + String(devicesFound));
  delay(1000);
  displayDeviceLocation("Distance: " + String(distance, 5) + " m " + "Location: " + getLocationOfOtherDevice(deviceName));
  devicesFound = 0;
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

  // GPS
  while (SerialGPS.available() > 0) {
    // Serial.println("SerialGPS > 0");
    if (gps.encode(SerialGPS.read()))
    {
      // Serial.println("SerialGPS.read() true");
      if (gps.location.isValid())
      {
        latitude = gps.location.lat();
        latitude_string = String(latitude , 6);
        longitude = gps.location.lng();
        longitude_string = String(longitude , 6);
        // Serial.print("Latitude = ");
        // Serial.println(latitude_string);
        // Serial.print("Longitude = ");
        // Serial.println(longitude_string);
        sendGPSLocationToDB(latitude_string, longitude_string);
      } else {
        sendGPSLocationToDB(latitude_string, longitude_string);
      }
      Serial.println();
    }
  }
  //delay(2000);
  Serial.println("");
  Serial.println("*****************SCAN DONE*****************");
  Serial.println("");
}
