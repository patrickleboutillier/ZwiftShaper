#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include "BLEProxy.h"


// Supported service UUIDs, as defined in GATT specifications
#define CPS_UUID                (uint16_t)0x1818
#define FMS_UUID                (uint16_t)0x1826

// Supported characteristics UUIDs, as defined in GATT specifications
#define CPS_CPF_UUID            (uint16_t)0x2A65
#define CPS_SL_UUID             (uint16_t)0x2A5D
#define CPS_CPM_UUID            (uint16_t)0x2A63
#define FMS_IB_UUID             (uint16_t)0x2AD2
#define FMS_FMCP_UUID           (uint16_t)0x2AD9

//Tacx FE-C service and characteristics
#define TACX_FEC_UUID           "6E40FEC1-B5A3-F393-E0A9-E50E24DCCA9E"
#define TACX_FEC_READ_UUID      "6E40FEC2-B5A3-F393-E0A9-E50E24DCCA9E"
#define TACX_FEC_WRITE_UUID     "6E40FEC3-B5A3-F393-E0A9-E50E24DCCA9E"


class ZwiftShaperCallbacks {
  public:
    virtual uint16_t onPower(uint16_t watts) = 0 ;
    virtual void onCadence(uint16_t rpms) = 0 ;
    virtual float onGrade(float grade) = 0 ;
    virtual void onTrainerDisconnect() = 0 ;
    virtual void onZwiftDisconnect() = 0 ;
} ;


class ZwiftShaper : public BLEAdvertisedDeviceCallbacks, public BLEProxyCallbacks {
  private:
    uint16_t prev_crevs, prev_lcet ;
    BLEAdvertisedDevice *remote_device ;
    ZwiftShaperCallbacks *callbacks ;

  public:
    ZwiftShaper(){
      prev_crevs = 0 ;
      prev_lcet = 0 ;
      remote_device = nullptr ;
    }


    void setCallbacks(ZwiftShaperCallbacks *c){
      callbacks = c ;
    }


    BLEAdvertisedDevice *getRemoteDevice(){
      return remote_device ;
    }
    

    void onResult(BLEAdvertisedDevice adev){
      // We have found a device, let us now see if it contains one of the services we are looking for.
      if ((adev.isAdvertisingService(BLEUUID(CPS_UUID)))||  // Cycling Power Service  
          (adev.isAdvertisingService(BLEUUID(CPS_UUID)))){  // Fitness Machine Service
        // For know, just pick the first one we find...
        Serial.print("- Found device '") ;
        Serial.print(adev.getName().c_str()) ;
        Serial.print("': ")  ;
        Serial.println(adev.toString().c_str()) ;
        Serial.println("  - Device with Cycling Power Service or Fitness Machine Service found: stopping scan") ;
        remote_device = new BLEAdvertisedDevice(adev) ;
        BLEDevice::getScan()->stop() ;
      }
    }


    std::string onRead(BLECharacteristic *c, std::string data){   
      return data ;
    }


    std::string onWrite(BLECharacteristic *c, std::string data){
      if (c->getUUID().equals(BLEUUID(TACX_FEC_WRITE_UUID))){ 
        return OnTacxFECWrite(c, data) ;
      }
      else if (c->getUUID().equals(BLEUUID(FMS_FMCP_UUID))){ 
        return OnFitnessMachineControlPoint(c, data) ;
      }
      return data ;
    }
    

    std::string onNotify(BLERemoteCharacteristic *rc, std::string data){
      if (rc->getUUID().equals(BLEUUID(CPS_CPM_UUID))){
        return onCyclingPowerMeasurement(rc, data) ;
      }
      else if (rc->getUUID().equals(BLEUUID(FMS_IB_UUID))){
        return onIndoorBike(rc, data) ;
      }
      return data ;
    }


    std::string onIndicate(BLERemoteCharacteristic *rc, std::string data){
      return data ;
    }


    std::string OnTacxFECWrite(BLECharacteristic *c, std::string s){
      std::vector<uint8_t> v(s.begin(), s.end()) ;
      uint8_t *data = &v[0] ;

      uint8_t sync = data[0] ;
      uint8_t len = data[1] ;
      uint8_t type = data[2] ;
      if ((sync == 0xA4)&&(len == 9)&&(type == 0x4E)){
        uint8_t page = data[4] ;
        if (page == 0x33){
          // Simulated Grade(%) = (Raw Grade Value x 0.01%) ??? 200.00%
          float grade = ((data[10] << 8 | data[9]) - 20000) / 100.0 ;
          if (callbacks){
            grade = callbacks->onGrade(grade) ;
          }
          uint16_t g = (uint16_t)((grade * 100.0) + 20000) ;
          data[10] = g >> 8 ;
          data[9] = g & 0xFF ;
        }
      }    
      
      return std::string(reinterpret_cast<const char *>(data), s.length()) ;
    }


    std::string OnFitnessMachineControlPoint(BLECharacteristic *c, std::string s){
      std::vector<uint8_t> v(s.begin(), s.end()) ;
      uint8_t *data = &v[0] ;
      switch(data[0]) {
        case 0x05:    // Target power level
          break ;
        case 0x11: {
          float grade = (int16_t)((data[4] << 8) + data[3]) / 100.0 ;
          Serial.print("-> FMCP Grade: ") ;
          Serial.print(grade) ; 
          Serial.println("%") ; 
          if (callbacks){
            grade = callbacks->onGrade(grade) ;
          }
          break ;
        }
        default:
          Serial.print("Unhandled FMCP data: ") ;
          Serial.println(data[0], HEX) ;
      }  
      
      return std::string(reinterpret_cast<const char *>(data), s.length()) ;  
    }


    std::string onCyclingPowerMeasurement(BLERemoteCharacteristic *rc, std::string s){
      std::vector<uint8_t> v(s.begin(), s.end()) ;
      uint8_t *data = &v[0] ;
      
      if (s.length() >= 4){
        uint8_t offset = 0 ;
        uint16_t flags = data[offset + 1] << 8 | data[offset] ;
        offset += 2 ;
        // Serial.print("Flags: 0b") ;
        // println(flags, BIN) ;
    
        uint16_t power = data[offset + 1] << 8 | data[offset] ;
        Serial.print("-> CPM Power: ") ;
        Serial.println(power) ;
        if (callbacks){
          power = callbacks->onPower(power) ;
        }
        data[offset + 1] = power >> 8 ;
        data[offset] = power & 0xFF ;
        offset += 2 ;
                    
        if (flags & 0b10000){
          uint32_t wrevs = data[offset+3] << 24 | data[offset+2] << 16 | data[offset+1] << 8 | data[offset] ;
          offset += 4 ;
          //Serial.print("Wheel Revolutions: ") ;
          //Serial.println(wrevs) ;
          uint16_t lwet = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;
        }
    
        if (flags & 0b100000){
          uint16_t crevs = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;
          // Serial.print("Crank Revolutions: ") ;
          // Serial.println(crevs) ;
          uint16_t lcet = data[offset + 1] << 8 | data[offset] ;
          offset += 2 ;

          uint16_t cr = crevs - prev_crevs ;
          if ((lcet > prev_lcet)&&(cr > 0)&&(prev_lcet > 0)){
            uint16_t dur = lcet - prev_lcet ;
            uint16_t rpms = (cr * 1024 * 60) / dur ;
            Serial.print("-> CPM Estimated RPMs: ") ;
            Serial.println(rpms) ;
            if (callbacks){
              callbacks->onCadence(rpms) ;
            }
          }
          prev_crevs = crevs ;
          prev_lcet = lcet ;
        }
      }

      return std::string(reinterpret_cast<const char *>(data), s.length()) ;
    }


    std::string onIndoorBike(BLERemoteCharacteristic *rc, std::string s){
      std::vector<uint8_t> v(s.begin(), s.end()) ;
      uint8_t *data = &v[0] ;
      
      if (s.length() >= 3){
        uint8_t offset = 0 ;
        uint16_t flags = data[offset + 1] << 8 | data[offset] ;
        // Serial.print("Flags: 0b") ;
        // Serial.println(flags, BIN) ;
        offset += 2 ;
        
        // speed, 2 bytes
        offset += 2 ;

        if (flags & 0b100){
          uint16_t rpms = (data[offset + 1] << 8 | data[offset]) / 2 ;
          Serial.print("-> IB RPMs: ") ;
          Serial.println(rpms) ;
          if (callbacks){
            callbacks->onCadence(rpms) ;
          }
          offset += 2 ;
        }
        if (flags & 0b1000000){
          uint16_t power = data[offset + 1] << 8 | data[offset] ;
          Serial.print("-> IB Power: ") ;
          Serial.println(power) ;
          if (callbacks){
            power = callbacks->onPower(power) ;
            data[offset + 1] = power >> 8 ;
            data[offset] = power & 0xFF ;
          }
          offset += 2 ;
        }
      }
      
      return std::string(reinterpret_cast<const char *>(data), s.length()) ;
    }

    
    void onTrainerDisconnect(){
      if (callbacks){
        callbacks->onTrainerDisconnect() ;
      }
    }


    void onZwiftDisconnect(){
      if (callbacks){
        callbacks->onZwiftDisconnect() ;
      }
    }
} ;


#endif
