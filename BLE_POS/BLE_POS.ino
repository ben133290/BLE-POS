// INCLUDES
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <cmath>

#include <string>

// DEFINES
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

#define BEACON_ONE_UUID "87bcbcc3-644e-4fb1-966e-23f9e4242112"
#define BEACON_ONE_MAJOR 8
#define BEACON_ONE_MINOR 1
#define BEACON_ONE_POWER -59
#define BEACON_ONE_POS_X 0
#define BEACON_ONE_POS_Y 0

#define BEACON_TWO_UUID "1155d39a-e93b-4099-92e0-fe05c818814e"
#define BEACON_TWO_MAJOR 8
#define BEACON_TWO_MINOR 2
#define BEACON_TWO_POWER -59
#define BEACON_TWO_POS_X 10
#define BEACON_TWO_POS_Y 10

#define BEACON_THREE_UUID "1d6d3cc6-b6b8-4cec-84f3-a7f56f84ffaa"
#define BEACON_THREE_MAJOR 8
#define BEACON_THREE_MINOR 3
#define BEACON_THREE_POWER -59
#define BEACON_THREE_POS_X 10
#define BEACON_THREE_POS_Y 0

int scanTime = 1; //In seconds
int numberOfVisibleBeacons = 0;
BLEScan *pBLEScan;

class PosBeacon {
  public:
    String uuid;
    int major;
    int minor;
    int power;
    bool visible;
    int rssi;
};

struct Point {
    double x;
    double y;
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      if (advertisedDevice.haveServiceUUID())
      {
        BLEUUID devUUID = advertisedDevice.getServiceUUID();
      }
      else
      {
        if (advertisedDevice.haveManufacturerData() == true)
        {
          std::string strManufacturerData = advertisedDevice.getManufacturerData();

          uint8_t cManufacturerData[100];
          strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

          if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
          {
            Serial.println("Found an iBeacon!");
            BLEBeacon oBeacon = BLEBeacon();
            oBeacon.setData(strManufacturerData);
            Serial.printf("ID: %04X \nMajor: %d \nMinor: %d \nUUID: %s \nPower: %d\n RSSI: %d\n", 
              oBeacon.getManufacturerId(),
              ENDIAN_CHANGE_U16(oBeacon.getMajor()),
              ENDIAN_CHANGE_U16(oBeacon.getMinor()),
              oBeacon.getProximityUUID().toString().c_str(),
              oBeacon.getSignalPower(),
              advertisedDevice.getRSSI());
          }
        }
        return;
      }
    }
};
/*
String determineLocation() {


  if (numberOfVisibleBeacons < 3) {
    // Determine Location based on Zones
  }
  else{
    // Determine Location based on triangulation
    
  }

}
*/

Point calculateObjectCoordinates(double distance1, double distance2, double distance3) {
    double x1 = BEACON_ONE_POS_X;
    double y1 = BEACON_ONE_POS_Y;
    double x2 = BEACON_TWO_POS_X;
    double y2 = BEACON_TWO_POS_Y;
    double x3 = BEACON_THREE_POS_X;
    double y3 = BEACON_THREE_POS_Y;

    double A = 2 * (x2 - x1);
    double B = 2 * (y2 - y1);
    double C = pow(distance1, 2) - pow(distance2, 2) - pow(x1, 2) + pow(x2, 2) - pow(y1, 2) + pow(y2, 2);
    double D = 2 * (x3 - x2);
    double E = 2 * (y3 - y2);
    double F = pow(distance2, 2) - pow(distance3, 2) - pow(x2, 2) + pow(x3, 2) - pow(y2, 2) + pow(y3, 2);

    double objectX = (C * E - F * B) / (E * A - B * D);
    double objectY = (C * D - A * F) / (B * D - A * E);

    Point objectCoordinates;
    objectCoordinates.x = objectX;
    objectCoordinates.y = objectY;

    return objectCoordinates;
}



void setup()
{
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(2000);
  
  Point point = calculateObjectCoordinates(10, 0, 0);
  Serial.printf("Calc Result x = %d, y = %d", point.x, point.y);
}
