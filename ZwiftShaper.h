#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include "ZSClient.h"


// Supported service UUIDs, as defined in GATT specifications
#define CPS_UUID                (uint16_t)0x1818
#define FMS_UUID                (uint16_t)0x1826

// Supported characteristics UUIDs, as defined in GATT specifications
#define CPS_CPF_UUID            (uint16_t)0x2A65
#define CPS_SL_UUID             (uint16_t)0x2A5D
#define CPS_CPM_UUID            (uint16_t)0x2A63


class ZwiftShaper {
  private:
    ZSClient *client ;
  public:
    ZwiftShaper(){
      BLEDevice::init("ZS[]") ;
      client = new ZSClient(this) ;
    }

    ZSClient *getZSClient(){
      return client ;
    }
} ;


#endif