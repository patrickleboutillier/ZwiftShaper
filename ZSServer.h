#ifndef ZSSERVER_H
#define ZSSERVER_H


#include "ZwiftShaper.h"


class ZSServer : public BLECharacteristicCallbacks, public BLEServerCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEServer *server ;
    BLEClient *client ;
    bool client_connected ;

  public:
    ZSServer(ZwiftShaper *zs, BLEServer *ble_server, BLEClient *ble_client) ;
    void startAdvertizing() ;
    void onConnect(BLEServer *srv) ;
    void onDisconnect(BLEServer *srv) ;
    void onRead(BLECharacteristic *chr) ;
    void onWrite (BLECharacteristic *chr) ;
    bool isClientConnected() ;
    BLEServer *getServer() ;
} ;


#endif
