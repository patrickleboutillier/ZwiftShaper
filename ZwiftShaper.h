#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>

#define GEN_ACC_UUID            (uint16_t)0x1800
#define GEN_ATTR_UUID           (uint16_t)0x1801

// Supported service UUIDs, as defined in GATT specifications
#define CPS_UUID                (uint16_t)0x1818
#define FMS_UUID                (uint16_t)0x1826

// Supported characteristics UUIDs, as defined in GATT specifications
#define CPS_CPF_UUID            (uint16_t)0x2A65
#define CPS_SL_UUID             (uint16_t)0x2A5D
#define CPS_CPM_UUID            (uint16_t)0x2A63


class ZSClient ; 
class ZSServer ; 

class ZwiftShaper {
  private:
    std::string name ;
    ZSClient *client ;
    ZSServer *server ;
  public:
    ZwiftShaper() ;
    ZSClient *getZSClient() ;
    ZSServer *getZSServer() ;
    BLEServer *getBLEServer() ;
    std::string getName() ;
    void setName(std::string n) ;
} ;


#endif
