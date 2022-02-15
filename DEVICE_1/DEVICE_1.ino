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
#define SERVICE_UUID        "37565ec6-46c8-11ec-81d3-0242ac130003"
#define CHARACTERISTIC_UUID "447cd81e-46c8-11ec-81d3-0242ac130003"
BLEScan* pBLEScan;

StaticJsonDocument<200> doc;
const char* ssid = "";
const char* password = "";
const String serverName = "";
const String serverNameGetDevice = "";
const int LED_WIFI = 5; // wifi led indicator
const int LED_POST_DATA = 2; // post data led indicator
const int BUZZER_NEAR_PERSON = 19; // near person indicator

int exposureCount = 0;
int scanTime = 3; //In seconds
int8_t N = 2; // index
int8_t A = -69; // indicates the signal strength is received from a reference node, normally at a distance of 1m.
// BLE uses Measured Power is also known as the 1 Meter RSSI. So consider the value of Measured Power = -69

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 5; // Set timer to 5 seconds (5000)

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
      doc["uid"] = serviceUUID;
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

      Serial.println(serverNameGetDevice + "/" + serviceUUID);

      int httpResponseCode = http.GET();
      String payload = "{}";
      payload = http.getString();

      if (httpResponseCode == 200) {
        if (payload != "null") { // if not null
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          Serial.println("Payload: " + payload);
          return true;
        } else { // else payload is "null"
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          Serial.println("Payload: " + payload);
          return false;
        }
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.println("Payload: " + payload);
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
      if (advertisedDevice.haveRSSI()) {
        float rssiOverTwo = (int)advertisedDevice.getRSSI() / 2; // for other device to work (idk)
        float ratio = (A - rssiOverTwo);
        float ratioOver20 = ratio / (10 * N);
        float distance = pow(10, ratioOver20); // in meters
        Serial.printf("Rssi-distance in meters: %f", distance);
        Serial.printf(" m \n");
        // Serial.printf("Rssi: %d \n", (int)advertisedDevice.getRSSI());

        // check if UID is existing in the database
        bool isExisting = checkInDatabase(advertisedDevice.toString().c_str());

        // Exposure count of 12 indicates 1 minute
        if (isExisting && distance <= 1 && exposureCount == 12) {
          digitalWrite(BUZZER_NEAR_PERSON, HIGH);
          digitalWrite (LED_WIFI, HIGH);
          delay(3000);
          digitalWrite(BUZZER_NEAR_PERSON, LOW);
          digitalWrite (LED_WIFI, LOW);
          postData(advertisedDevice.toString().c_str());
          exposureCount = 0;
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_POST_DATA, OUTPUT);
  pinMode(BUZZER_NEAR_PERSON, OUTPUT);

  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" with password ");
  Serial.println(password);

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
  Serial.println("Devices found: ");
  Serial.println(foundDevices.getCount());

  if (foundDevices.getCount() == 0) {
    exposureCount = 0;
  }
  
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    if (strcmp(device.getName().c_str(), "DEVICE 2") == 0) {
      exposureCount++;
      Serial.println(device.getName().c_str());
      Serial.println(exposureCount);
    }
  }
  Serial.println("Scan done!");
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
  Serial.println("*******************************");
}
