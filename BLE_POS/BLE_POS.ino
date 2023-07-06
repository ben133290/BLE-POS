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

#ifndef EPSILON
#define EPSILON 6
#endif

#define BEACON_ONE_UUID  "122124e4-f923-6e96-b14f-4e64c3bcbc87" //Maria main Phone // "87bcbcc3-644e-4fb1-966e-23f9e4242112"
#define BEACON_ONE_MAJOR 8
#define BEACON_ONE_MINOR 1
#define BEACON_ONE_POWER -59
#define BEACON_ONE_POS_X 8
#define BEACON_ONE_POS_Y 10

#define BEACON_TWO_UUID "4e8118c8-05fe-e092-9940-3be99ad35511"  // Maria Tablet
#define BEACON_TWO_MAJOR 8
#define BEACON_TWO_MINOR 2
#define BEACON_TWO_POWER -59
#define BEACON_TWO_POS_X 0
#define BEACON_TWO_POS_Y 0

#define BEACON_THREE_UUID "aaff846f-f5a7-f384-ec4c-b8b6c63c6d1d" // Maria old Phone
#define BEACON_THREE_MAJOR 8
#define BEACON_THREE_MINOR 3
#define BEACON_THREE_POWER -59
#define BEACON_THREE_POS_X 16
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

bool calculateThreeCircleIntersection(double x0, double y0, double r0,
                                      double x1, double y1, double r1,
                                      double x2, double y2, double r2,
                                      double* result_x, double* result_y)
{
  double a, dx, dy, d, h, rx, ry;
  double point2_x, point2_y;

  /* dx and dy are the vertical and horizontal distances between
   * the circle centers.
   */
  dx = x1 - x0;
  dy = y1 - y0;

  /* Determine the straight-line distance between the centers. */
  d = sqrt((dy * dy) + (dx * dx));

  /* Check for solvability. */
  if (d > (r0 + r1))
  {
    /* no solution. circles do not intersect. */
    // Serial.println("no solution. circles do not intersect.");
    return false;
  }
  if (d < abs(r0 - r1))
  {
    // Serial.println("no solution. one circle is contained in the other");
    return false;
  }

  /* 'point 2' is the point where the line through the circle
   * intersection points crosses the line between the circle
   * centers.
   */

  /* Determine the distance from point 0 to point 2. */
  a = ((r0 * r0) - (r1 * r1) + (d * d)) / (2.0 * d);

  /* Determine the coordinates of point 2. */
  point2_x = x0 + (dx * a / d);
  point2_y = y0 + (dy * a / d);

  /* Determine the distance from point 2 to either of the
   * intersection points.
   */
  h = sqrt((r0 * r0) - (a * a));

  /* Now determine the offsets of the intersection points from
   * point 2.
   */
  rx = -dy * (h / d);
  ry = dx * (h / d);

  /* Determine intersection points. */
  double intersecPoint1_x = point2_x + rx;
  double intersecPoint2_x = point2_x - rx;
  double intersecPoint1_y = point2_y + ry;
  double intersecPoint2_y = point2_y - ry;

  /* Lets determine if circle 3 intersects at either of the above intersection points. */
  dx = intersecPoint1_x - x2;
  dy = intersecPoint1_y - y2;
  double d1 = sqrt((dy * dy) + (dx * dx));

  dx = intersecPoint2_x - x2;
  dy = intersecPoint2_y - y2;
  double d2 = sqrt((dy * dy) + (dx * dx));

  if (abs(d1 - r2) < EPSILON)
  {
    *result_x = intersecPoint1_x;
    *result_y = intersecPoint1_y;
    return true;
  }
  else if (abs(d2 - r2) < EPSILON)
  {
    *result_x = intersecPoint2_x;
    *result_y = intersecPoint2_y;
    return true;
  }

  /* The individual distances between the intersection points and the radius of the third beacon are tot great. */
  return false;
}

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
            // Serial.printf("FOUND TABLET\n");
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

    /* Scale Radii */
    double r1 = (abs(RSSIOne) - 47) / 7;
    Serial.printf("Radius 1: %f\n", r1);
    if (r1 < 0) { r1 = 0; }
    double r2 = (abs(RSSITwo) - 50) / 5;
    if (r2 < 0) { r2 = 0; }
    Serial.printf("Radius 2: %f\n", r2);
    double r3 = (abs(RSSIThree) - 30) / 7;
    if (r3 < 0) { r3 = 0; }
    Serial.printf("Radius 3: %f\n", r3);

    bool worked = calculateThreeCircleIntersection(BEACON_ONE_POS_X, BEACON_ONE_POS_Y, r1,
                                     BEACON_TWO_POS_X, BEACON_TWO_POS_Y, r2,
                                     BEACON_THREE_POS_X, BEACON_THREE_POS_Y, r3,
                                     &result_x, &result_y);

    Serial.printf("-----------------\n");
    if (worked) {
    Serial.printf("Position found: (%f, %f)\n", result_x, result_y);
    } else {
      bool worked2 = calculateThreeCircleIntersection(BEACON_ONE_POS_X, BEACON_ONE_POS_Y, r1+1,
                                     BEACON_TWO_POS_X, BEACON_TWO_POS_Y, r2+1,
                                     BEACON_THREE_POS_X, BEACON_THREE_POS_Y, r3+1,
                                     &result_x, &result_y);
      if (!worked2) {
      Serial.println("Couldn't Calculate Intersection!");
      } else {
        Serial.printf("Position found: (%f, %f)\n", result_x, result_y);
      }
    }
    Serial.printf("-----------------\n");


  }

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


  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  numberOfVisibleBeacons = 0;
  RSSIOne = -1000;
  RSSITwo = -1000;
  RSSIThree = -1000;
  delay(2000);
}
