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

    void setupServices(){
      BLEService *cps = server->createService(BLEUUID(CPS_UUID)) ;
      cps->addCharacteristic(&cycling_power_feature) ;
      cps->addCharacteristic(&sensor_location) ;
      cps->addCharacteristic(&cycling_power_measurement) ;
      cps->start() ;
    }

    // Callback called on client connect
    void onConnect(BLEServer *srv) {
      client_connected = true ;
    }

    // Callback called on client disconnect
    void onDisconnect(BLEServer *srv) {
      client_connected = false ;
    }

    void onRead(BLECharacteristic *chr){
    }
 
    void onWrite (BLECharacteristic *chr){
    }
} ;


#endif
