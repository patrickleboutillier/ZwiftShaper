#ifndef ZSCLIENT_H
#define ZSCLIENT_H

#include "ZwiftShaper.h"


class ZSClient : public BLEAdvertisedDeviceCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEClient *client ;
    BLEAdvertisedDevice *remote_device ;

  public:
    ZSClient(ZwiftShaper *zs){
      shaper = zs ;
      client = BLEDevice::createClient() ;
    }

    // Scans and locates the desired server
    bool findServer(int timeout){
      Serial.println("Starting scan...") ;
      BLEScan* scan = BLEDevice::getScan() ;
      scan->setAdvertisedDeviceCallbacks(this) ;
      scan->setActiveScan(true) ;
      scan->start(timeout) ; 

      // The callbacks will set remote_device if a proper device (.i.e. BLE Server) is found.
      // At this point we are not yet connected to this device though.
      return (remote_device != nullptr) ;
    }

    // Called while scanning when a new device is discovered.
    void onResult(BLEAdvertisedDevice adev) {
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

    // Connect to the BLE Server (i.e. the "trainer") at the specified address
    bool connectToServer() {
      if (remote_device == nullptr){
        return false ;
      }

      Serial.print("Connecting to remote device '") ;
      Serial.print(remote_device->getName().c_str()) ;
      Serial.println("'")  ;
      // Connect to the remote BLE Server.
      client->connect(remote_device) ;
                  
      return true ;
    }

    void setupServices(){
      // Now we want to get all the services offered by the device, with their characteristics, 
      // register them with our server and setup callbacks for the ones we want to influence.
      std::map<std::string, BLERemoteService*> *srvcmap = client->getServices() ;
      std::map<std::string, BLERemoteService*>::iterator it ;
      for (it = srvcmap->begin() ; it != srvcmap->end() ; it++){
        std::string uuid = it->first ;
        BLERemoteService *remsrvc = it->second ;
        Serial.print("- Found service '") ;
        Serial.print(uuid.c_str()) ;
        Serial.println("'") ;

        // Setup this service on our server
        BLEService *srvc = shaper->getBLEServer()->createService(remsrvc->getUUID()) ;

        std::map<std::string, BLERemoteCharacteristic*> *chrsmap = remsrvc->getCharacteristics() ;
        std::map<std::string, BLERemoteCharacteristic*>::iterator it ;
        for (it = chrsmap->begin() ; it != chrsmap->end() ; it++){
          std::string uuid = it->first ;
          BLERemoteCharacteristic *remchr = it->second ;
          Serial.print("  - Found characteristic :") ;
          Serial.println(remchr->toString().c_str()) ;
          uint32_t properties =
            (remchr->canRead() ? BLECharacteristic::PROPERTY_READ : 0) |
            (remchr->canWrite() ? BLECharacteristic::PROPERTY_WRITE : 0) |
            (remchr->canNotify() ? BLECharacteristic::PROPERTY_NOTIFY : 0) |
            (remchr->canBroadcast() ? BLECharacteristic::PROPERTY_BROADCAST : 0) |
            (remchr->canIndicate() ? BLECharacteristic::PROPERTY_INDICATE : 0) |
            (remchr->canWriteNoResponse() ? BLECharacteristic::PROPERTY_WRITE_NR : 0) ;
      
          // Setup this characteristic on our server
          BLECharacteristic *chr = new BLECharacteristic(BLEUUID(remchr->getUUID()), properties) ;
          pFitness->addCharacteristic(chr) ;

          if (remchr->canNotify()){
            // We need to set up a callback to handle these notifications
            remchr->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
              if (blerc->getUUID().equals(BLEUUID(CPS_CPM_UUID))){
                this->onCyclingPowerMeasurement(blerc, data, length, is_notify) ;
              }

              // Now that we had a chance to modify the data, we must forward it through our server counterpart:
              chr.setValue(data, length) ;
              chr.notify() ;
            }) ;
          }
        }

        remsrvc->start() ;
      }
    }
    
    void disconnectFromServer(){
      Serial.print("Disconnecting to remote device") ;
      client->disconnect() ;
    }

    void onCyclingPowerMeasurement(BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
      if (length >= 4){
        uint8_t offset = 0 ;
        uint16_t flags = data[offset + 1] << 8 | data[offset] ;
        offset += 2 ;
        Serial.print("Flags: 0b") ;
        Serial.println(flags, BIN) ;

        int16_t power = data[offset + 1] << 8 | data[offset] ;
        offest += 2 ;
        Serial.print("Power: ") ;
        Serial.print(power) ;
        Serial.println("w ") ;

        if (flags & 0b10000){
          uint32_t wrevs = data[offset+3] << 24 | data[offset+2] << 16 | data[offset+1] << 8 | data[offset] ;
          offest += 4 ;
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
