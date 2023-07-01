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
#include "positioning.h"

#include <string>

// DEFINES
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

#define BEACON_ONE_UUID  "122124e4-f923-6e96-b14f-4e64c3bcbc87" //Maria main Phone // "87bcbcc3-644e-4fb1-966e-23f9e4242112"
#define BEACON_ONE_MAJOR 8
#define BEACON_ONE_MINOR 1
#define BEACON_ONE_POWER -59
#define BEACON_ONE_POS_X 0
#define BEACON_ONE_POS_Y 0

#define BEACON_TWO_UUID "4e8118c8-05fe-e092-9940-3be99ad35511"  // Maria Tablet
#define BEACON_TWO_MAJOR 8
#define BEACON_TWO_MINOR 2
#define BEACON_TWO_POWER -59
#define BEACON_TWO_POS_X 100
#define BEACON_TWO_POS_Y 100

#define BEACON_THREE_UUID "aaff846f-f5a7-f384-ec4c-b8b6c63c6d1d" // Maria old Phone
#define BEACON_THREE_MAJOR 8
#define BEACON_THREE_MINOR 3
#define BEACON_THREE_POWER -59
#define BEACON_THREE_POS_X 100
#define BEACON_THREE_POS_Y 0

int scanTime = 1; // In seconds
int numberOfVisibleBeacons = 0;
int RSSIOne = -1000;
int RSSITwo = -1000;
int RSSIThree = -1000;
BLEScan *pBLEScan;

class PosBeacon
{
public:
  String uuid;
  int major;
  int minor;
  int power;
  bool visible;
  int rssi;
};

struct Point
{
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
          // Serial.println("Found an iBeacon!");
          BLEBeacon oBeacon = BLEBeacon();
          oBeacon.setData(strManufacturerData);
          
          /*Serial.printf("ID: %04X \nMajor: %d \nMinor: %d \nUUID: %s \nPower: %d\n RSSI: %d\n",
                        oBeacon.getManufacturerId(),
                        ENDIAN_CHANGE_U16(oBeacon.getMajor()),
                        ENDIAN_CHANGE_U16(oBeacon.getMinor()),
                        oBeacon.getProximityUUID().toString().c_str(),
                        oBeacon.getSignalPower(),
                        advertisedDevice.getRSSI());*/
          
          // check which of the position iBeacons can be seen
          String beaconName = oBeacon.getProximityUUID().toString().c_str();
          int beaconRSSI = advertisedDevice.getRSSI();
          if(beaconName.compareTo(BEACON_ONE_UUID)== 0) {
            //Serial.printf("FOUND MAIN PHONE\n");
            numberOfVisibleBeacons++;
            RSSIOne = beaconRSSI;
            // Serial.printf("RSSI 1: %i\n", RSSIOne);
          }
          if(beaconName.compareTo(BEACON_TWO_UUID)==0) {
            //Serial.printf("FOUND TABLET\n");
            numberOfVisibleBeacons++;
            RSSITwo = beaconRSSI;
            // Serial.printf("RSSI 2: %i\n", RSSITwo);
          }
          if(beaconName.compareTo(BEACON_THREE_UUID)==0) {
            //Serial.printf("FOUND OLD PHONE\n");
            numberOfVisibleBeacons++;
            RSSIThree = beaconRSSI;
            // Serial.printf("RSSI 3: %i\n", RSSIThree);
          }

          //Serial.printf("BEACONS: %i\n", numberOfVisibleBeacons);
        }
      }
      return;
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop()
{
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  //Serial.print("Devices found: ");
  //Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

  // check how many position iBeacons have been found and acts accordingly
  if (numberOfVisibleBeacons >= 3) {

    /* Intialize vars for position result */
    double result_x = 0;
    double result_y = 0;
    double r1 = abs(RSSIOne);
    double r2 = abs(RSSITwo);
    double r3 = abs(RSSIThree);
    calculateThreeCircleIntersection(BEACON_ONE_POS_X, BEACON_ONE_POS_Y, r1,
                                     BEACON_TWO_POS_X, BEACON_TWO_POS_Y, r2,
                                     BEACON_THREE_POS_X, BEACON_THREE_POS_Y, r3,
                                     &result_x, &result_y);
    Serial.printf("-----------------\n");
    Serial.printf("Position found: (%d, %d)\n", result_x, result_y);
    Serial.printf("-----------------\n");


  } else {
    if (RSSIOne >= RSSITwo && RSSIOne >= RSSIThree) {
      Serial.printf("-----------------\n");
      Serial.printf("You're in Zone 1 (Maria main Phone)\n");
      Serial.printf("-----------------\n");
    } else if (RSSITwo >= RSSIOne && RSSITwo >= RSSIThree) {
      Serial.printf("-----------------\n");
      Serial.printf("You're in Zone 2 (Maria Tablet)\n");
      Serial.printf("-----------------\n");
    } else if (RSSIThree >= RSSIOne && RSSIThree >= RSSITwo) {
      Serial.printf("-----------------\n");
      Serial.printf("You're in Zone 3 (Maria old Phone)\n");
      Serial.printf("-----------------\n");
    } else if (RSSIOne == RSSITwo && RSSITwo == RSSIThree) {
      Serial.printf("-----------------\n");
      Serial.printf("Your position can't be determined\n");
      Serial.printf("-----------------\n");
    }
  }

  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  numberOfVisibleBeacons = 0;
  RSSIOne = -1000;
  RSSITwo = -1000;
  RSSIThree = -1000;
  delay(2000);
}
