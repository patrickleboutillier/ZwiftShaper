#include "ZwiftShaper.h"


ZwiftShaper *ZS = nullptr ;
BLEProxy *PROXY = nullptr ;


class MyZwiftShaperCallbacks : public ZwiftShaperCallbacks {
    // Called when trainer send power value to game
    int16_t onPower(int16_t watts){
      return 120 ;
    }
    // Called when game sends grade value to trainer
    float onGrade(float grade){
      Serial.print("Grade: ") ;
      Serial.println(grade) ;
      return grade ;
    }
} ;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;

  ZS = new ZwiftShaper() ;
  ZS->setCallbacks(new MyZwiftShaperCallbacks()) ;
  PROXY = new BLEProxy("ZS") ;
  PROXY->setCallbacks(ZS) ;
  
  Serial.println("Starting scan...") ;
  BLEScan *scan = BLEDevice::getScan() ;
  scan->setAdvertisedDeviceCallbacks(ZS) ;
  scan->setActiveScan(true) ;
  scan->start(15) ;

  BLEAdvertisedDevice *rd = ZS->getRemoteDevice() ;
  if (rd != nullptr){
    PROXY->cloneBLEProfile(rd) ;

    Serial.println("Starting advertisement...") ;
    BLEDevice::startAdvertising() ;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  PROXY->processEvents() ;
}
