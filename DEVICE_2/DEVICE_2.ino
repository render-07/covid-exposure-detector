//////////DEVICE 2////////////
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
#define SERVICE_UUID        "9ecc3b20-46c8-11ec-81d3-0242ac130003"
#define CHARACTERISTIC_UUID "afda8bce-46c8-11ec-81d3-0242ac130003"

// For a connection via I2C using the Arduino Wire include:
#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h"   // legacy: #include "SH1106.h"
SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

BLEScan* pBLEScan;

StaticJsonDocument<200> doc;
const char* ssid = "wifiName";
const char* password = "wifiPassword";
const String serverName = "localhost:port/api/information";
const String serverNameGetDevice = "localhost:port/api/devices";
const int LED_WIFI = 5; // wifi led indicator
const int LED_POST_DATA = 2; // post data led indicator
const int BUZZER_NEAR_PERSON = 19; // near person indicator
const String DEVICE_NAME = "DEVICE 2";
int exposureCount = 0;
int scanTime = 3; //In seconds
int devicesFound = 0;
int8_t N = 2; // index
int8_t A = -69; // indicates the signal strength is received from a reference node, normally at a distance of 1m.

// BLE uses Measured Power is also known as the 1 Meter RSSI. So consider the value of Measured Power = -69

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 5; // Set timer to 5 seconds (5000)

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
      doc["closeContact"] = serviceUUID;
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
          // Serial.print("HTTP Response code: ");
          // Serial.println(httpResponseCode);
          // Serial.println("Payload: " + payload);
          return true;
        } else { // else payload is "null"
          // Serial.print("HTTP Response code: ");
          // Serial.println(httpResponseCode);
          // Serial.println("Payload: " + payload);
          Serial.println("ID not existing in DB");
          return false;
        }
      }
      else {
        refreshDisplay("No connection to the server");
        // Serial.print("Error code: ");
        // Serial.println(httpResponseCode);
        // Serial.println("Payload: " + payload);
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

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

      // check if UID is existing in the database
      bool isExistingInDb = checkInDatabase(advertisedDevice.toString().c_str());

      if (advertisedDevice.haveRSSI() && isExistingInDb) {
        // float rssi = (int)advertisedDevice.getRSSI() / 2; // for other device to work (idk)
        // float rssi = (int)advertisedDevice.getRSSI();
        // float ratio = (A - rssi);
        // float ratioOver20 = ratio / (20);
        // // Serial.println(String(rssi));
        // // Serial.println("ratio" + String(ratio));
        // // Serial.println("ratioOver20" + String(ratioOver20));

        // float distance = pow(10, ratioOver20); // in meters
        // Serial.println(String(distance, 5));
        // Serial.printf("Rssi-distance in meters: %e", distance);
        // Serial.printf("Rssi-distance in meters: %f", (distance));
        // Serial.printf(" m \n");
        // Serial.printf("Rssi: %d \n", (int)advertisedDevice.getRSSI());

        // displayDeviceFound("Distance: " + String(distance, 5) + " m");

        // if (isExistingInDb) {
        //   devicesFound++;
        // }

        devicesFound++;
        Serial.println("Devices found: ");
        Serial.println(devicesFound);
        displayDeviceFound("Devices found: " + String(devicesFound));

        // Exposure count of 12 indicates 1 minute (180 == 15 minute)
        if (exposureCount == 1) {
          digitalWrite(BUZZER_NEAR_PERSON, HIGH);
          digitalWrite (LED_WIFI, HIGH);
          delay(3000);
          digitalWrite(BUZZER_NEAR_PERSON, LOW);
          digitalWrite (LED_WIFI, LOW);
          postData(advertisedDevice.toString().c_str());
          exposureCount = 0;
        }
      }
      Serial.println("Devices found: ");
      Serial.println(devicesFound);
      displayDeviceFound("Devices found: " + String(devicesFound));
    }
};

void setup() {
  Serial.begin(115200);
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

  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" with password ");
  Serial.println(password);

  refreshDisplay("Connecting to " + String(ssid));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite (LED_WIFI, HIGH);
    delay(50);
    digitalWrite (LED_WIFI, LOW);
    delay(50);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  refreshDisplay("Connected to WiFi network with IP Address: " + String(WiFi.localIP().toString().c_str()));

  // Server part
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue("From " + DEVICE_NAME);
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
  // BLEDevice::init("DEVICE 2");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);

  if (foundDevices.getCount() == 0) {
    exposureCount = 0;
  }

  // Try after scanning do the operation
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    if (strcmp(device.getName().c_str(), "DEVICE 1") == 0) {
      Serial.println(device.getName().c_str());

      // check if UID is existing in the database
      bool isExistingInDb = checkInDatabase(device.toString().c_str());

      if (device.haveRSSI() && isExistingInDb) {
        Serial.printf("Rssi: %d \n", (int)device.getRSSI());
        // float rssi = (int)advertisedDevice.getRSSI() / 2; // for other device to work (idk)
        float rssi = (int)device.getRSSI() + 20;
        float ratio = (A - rssi);
        float ratioOver20 = ratio / (20);
        // Serial.println(String(rssi));
        // Serial.println("ratio" + String(ratio));
        // Serial.println("ratioOver20" + String(ratioOver20));

        float distance = pow(10, ratioOver20); // in meters
        Serial.printf("Rssi-distance in meters: %f", (distance));
        Serial.printf(" m \n");

        displayDeviceFound("Distance: " + String(distance, 5) + " m");

        if (distance <= 1) {
          exposureCount++;
        }
        Serial.println(exposureCount);
      }
    }
  }

  Serial.println("Scan done!");
  devicesFound = 0;

  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
  Serial.println("*******************************");
}
