#include "ZwiftShaper.h"


ZwiftShaper *ZS = nullptr ;
BLEProxy *PROXY = nullptr ;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;

  ZS = new ZwiftShaper() ;
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
