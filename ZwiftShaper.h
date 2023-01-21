#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include "BLEProxy.h"


#define GEN_ACC_UUID            (uint16_t)0x1800
#define GEN_ATTR_UUID           (uint16_t)0x1801

// Supported service UUIDs, as defined in GATT specifications
#define CPS_UUID                (uint16_t)0x1818
#define FMS_UUID                (uint16_t)0x1826

// Supported characteristics UUIDs, as defined in GATT specifications
#define CPS_CPF_UUID            (uint16_t)0x2A65
#define CPS_SL_UUID             (uint16_t)0x2A5D
#define CPS_CPM_UUID            (uint16_t)0x2A63


class ZwiftShaper : public BLEAdvertisedDeviceCallbacks, public BLEProxyCallbacks {
  private:
    BLEAdvertisedDevice *remote_device ;
  public:
    ZwiftShaper(){
      remote_device = nullptr ;
    }

    BLEAdvertisedDevice *getRemoteDevice(){
      return remote_device ;
    }
    
    void onResult(BLEAdvertisedDevice adev){
      // We have found a device, let us now see if it contains one of the services we are looking for.
      if (adev.isAdvertisingService(BLEUUID(CPS_UUID))){ // Cycling Power Service
        // For know, just pick the first one we find...
        Serial.print("- Found device '") ;
        Serial.print(adev.getName().c_str()) ;
        Serial.print("': ")  ;
        Serial.println(adev.toString().c_str()) ;
        Serial.println("  - Device with Cycling Power Service found: stopping scan") ;
        remote_device = new BLEAdvertisedDevice(adev) ;
        BLEDevice::getScan()->stop() ;
      }
    }


    std::string onRead(BLECharacteristic *c, std::string data){   
      return data ;
    }

    
    void onNotify(BLERemoteCharacteristic *rc, uint8_t *data, size_t length){
    }

    
    void onDisconnect(bool server_connected, bool client_connected){
      ESP.restart() ;
    }


    void onCyclingPowerMeasurement(BLERemoteCharacteristic *rc, uint8_t *data, size_t length, bool is_notify){
      if (length >= 4){
        uint8_t offset = 0 ;
        uint16_t flags = data[offset + 1] << 8 | data[offset] ;
        offset += 2 ;
        Serial.print("Flags: 0b") ;
        Serial.println(flags, BIN) ;
    
        int16_t power = data[offset + 1] << 8 | data[offset] ;
        offset += 2 ;
        Serial.print("Power: ") ;
        Serial.print(power) ;
        Serial.println("w ") ;
    
        if (flags & 0b10000){
          uint32_t wrevs = data[offset+3] << 24 | data[offset+2] << 16 | data[offset+1] << 8 | data[offset] ;
          offset += 4 ;
          Serial.print("Wheel Revolutions: ") ;
          Serial.println(wrevs) ;
          uint16_t lwet = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;
        }
    
        if (flags & 0b100000){
          uint32_t crevs = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;
          Serial.print("Crank Revolutions: ") ;
          Serial.println(crevs) ;
          uint16_t lwet = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;
        }
      }
    }
} ;


#endif
