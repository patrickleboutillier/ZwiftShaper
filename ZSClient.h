#ifndef ZSCLIENT_H
#define ZSCLIENT_H


#include "ZwiftShaper.h"


class ZSClient : public BLEAdvertisedDeviceCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEClient *client ;
    BLEServer *server ;
    BLEAdvertisedDevice *remote_device ;

  public:
    ZSClient(ZwiftShaper *zs, BLEClient *ble_client, BLEServer *ble_server) ;
    bool findServer(int timeout) ;
    void onResult(BLEAdvertisedDevice adev) ;
    bool connectToServer() ;
    void setupServices() ;
    void disconnectFromServer() ;
    void onCyclingPowerMeasurement(BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify) ;
} ;


#endif
