#ifndef ZSSERVER_H
#define ZSSERVER_H

#include "ZwiftShaper.h"


// Characteristic definitions
BLECharacteristic cycling_power_feature(BLEUUID(CPS_CPF_UUID), BLECharacteristic::PROPERTY_READ) ;
BLECharacteristic sensor_location(BLEUUID(CPS_SL_UUID), BLECharacteristic::PROPERTY_READ) ;
BLECharacteristic cycling_power_measurement(BLEUUID(CPS_CPM_UUID), BLECharacteristic::PROPERTY_NOTIFY) ;


class ZSServer : public BLECharacteristicCallbacks, public BLEServerCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEServer *server ;
    BLEUUID service_bleuuid ;
    BLEService *service ;
    bool client_connected ;

  public:
    ZSServer(ZwiftShaper *zs){
      shaper = zs ;
      server = BLEDevice::createServer() ;
      server->setCallbacks(this) ;
      service = nullptr ;
      client_connected = false ;
    }

    void startAdvertizing(){
      // ::esp_ble_gap_set_device_name(deviceName.c_str()) ;
      BLEAdvertising *adv = BLEDevice::getAdvertising() ;
      adv->setScanResponse(true) ;
      adv->setMinPreferred(0x06) ;
      BLEDevice::startAdvertising() ;
    }

    // Callback called on client connect
    void onConnect(BLEServer *srv) {
      Serial.println("Connected") ;
      client_connected = true ;
    }

    // Callback called on client disconnect
    void onDisconnect(BLEServer *srv) {
      Serial.println("Disconnected") ;
      client_connected = false ;
      // BLEDevice::startAdvertising() ;
    }

    void onRead(BLECharacteristic *chr){
    }
 
    void onWrite (BLECharacteristic *chr){
    }

    BLEServer *getServer(){
      return server ;
    }
} ;


#endif
