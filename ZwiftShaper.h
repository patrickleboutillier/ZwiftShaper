#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <BLEDevice.h>
#include <BLEUtils.h>

// Supported service UUIDs, as defined in GATT specifications
#define CPS_UUID                (uint16_t)0x1818
#define FMS_UUID                (uint16_t)0x1826

// Supported characteristics UUIDs, as defined in GATT specifications
#define CPS_CPF_UUID            (uint16_t)0x2A65
#define CPS_SL_UUID             (uint16_t)0x2A5D
#define CPS_CPM_UUID            (uint16_t)0x2A63


class ZwiftShaper ; 
#include "ZSClient.h"
#include "ZSServer.h"

class ZwiftShaper {
  private:
    std::string name ;
    ZSClient *client ;
    ZSServer *server ;
  public:
    ZwiftShaper(){
      name = "ZS[]" ;
      BLEDevice::init(name.c_str()) ;
      client = new ZSClient(this) ;
      server = new ZSServer(this) ;
    }

    ZSClient *getZSClient(){
      return client ;
    }

    ZSServer *getZSServer(){
      return server ;
    }

    BLEServer *getBLEServer(){
      return server->getServer() ;
    }

    const char *getName(){
      return name.c_str() ;
    }

    void setName(const char *n){
      name = n ;
    }
} ;


#endif
