#ifndef ZSSERVER_H
#define ZSSERVER_H


#include "ZwiftShaper.h"


class ZSServer : public BLECharacteristicCallbacks, public BLEServerCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEServer *server ;
    bool client_connected ;

  public:
    ZSServer(ZwiftShaper *zs) ;
    void startAdvertizing() ;
    void onConnect(BLEServer *srv) ;
    void onDisconnect(BLEServer *srv) ;
    void onRead(BLECharacteristic *chr) ;
    void onWrite (BLECharacteristic *chr) ;
    bool isClientConnected() ;
    BLEServer *getServer() ;
} ;


#endif
