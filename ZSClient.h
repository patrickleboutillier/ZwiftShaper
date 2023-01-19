#ifndef ZSCLIENT_H
#define ZSCLIENT_H

#include <ZwiftShaper.h>


class ZSClient : public BLEAdvertisedDeviceCallbacks, public BLEClientCallbacks {
  private:
    ZwiftShaper *shaper ;
    BLEClient *client ;
    BLEAdvertisedDevice *remote_device ;
    BLEUUID remote_service_bleuuid ;
    BLERemoteService *remote_service ;

  public:
    ZSClient(ZwiftShaper *zs){
      shaper = zs ;
      client = BLEDevice::createClient() ;
      remote_device = nullptr ;
      remote_service = nullptr ;
    }

    // Scans and locates the desired server
    bool findServer(int timeout){
      Serial.println("Starting scan...") ;
      BLEScan* pBLEScan = BLEDevice::getScan() ;
      pBLEScan->setAdvertisedDeviceCallbacks(this) ;
      pBLEScan->setActiveScan(true) ;
      pBLEScan->start(timeout) ; 

      // The callbacks will set remote_device if a proper device (.i.e. BLE Server) is found.
      // At this point we are not yet connected to this device though.
      return (remote_device != nullptr) ;
    }

    // Called while scanning when a new device is discovered.
    void onResult(BLEAdvertisedDevice adev) {
      Serial.print("- Found device '") ;
      Serial.print(adev.getName().c_str()) ;
      Serial.print("': ")  ;
      Serial.println(adev.toString().c_str()) ;
      // We have found a device, let us now see if it contains one of the services we are looking for.
      if (adev.isAdvertisingService(BLEUUID(CPS_UUID))){ // Cycling Power Service
        // For know, just pick the first one we find...
        Serial.println("  - Device with Cycling Power Service found: stopping scan") ;
        remote_service_bleuuid = BLEUUID(CYCLING_POWER_SERVICE) ;
        remote_device = new BLEAdvertisedDevice(adev) ;
        BLEDevice::getScan()->stop() ;
      }
    }

    // Connect to the BLE Server (i.e. the "trainer") at the specified address
    bool connectToServer() {
      if (remote_device != nullptr){
        return false ;
      }

      Serial.print("Connecting to remote device '") ;
      Serial.print(remote_device->getName().c_str()) ;
      Serial.println("'")  ;
      // Connect to the remove BLE Server.
      client->connect(remote_device) ;
                  
      // Obtain a reference to the service we are after in the remote BLE server.
      remote_service = client->getService(remote_service_bleuuid) ;
      if (remote_service != nullptr) {
        Serial.println("- Found remote service") ;
      } 

      if (remote_service_bleuuid.equals(BLEUUID(CYCLING_POWER_SERVICE))){
        setupForCyclingPowerService() ;
      }

      return true ;
    }

    void disconnectFromServer(){
      Serial.print("Disconnecting to remote device") ;
      client->disconnect() ;
    }

    void setupForCyclingPowerService(){
      BLERemoteCharacteristic *cycling_power_feature = remote_service->getCharacteristic(BLEUUID(CPS_CPF_UUID)) ;
      if (cycling_power_feature != nullptr) {
        Serial.println("  - Found Cycling Power Feature characteristic") ;
      }     
      uint32_t cpf = cycling_power_feature->readUInt32() ;
      Serial.print("Feature: 0b") ;
      Serial.println(cpf, BIN) ;

      BLERemoteCharacteristic *sensor_location = remote_service->getCharacteristic(BLEUUID(CPS_SL_UUID)) ;
      if (sensor_location != nullptr) {
        Serial.println("  - Found Sensor Location characteristic") ;
      }
      uint8_t sl = sensor_location->readUInt8() ;
      Serial.print("Location: 0x") ;
      Serial.println(sl, HEX) ;
      
      BLERemoteCharacteristic *cycling_power_measurement = remote_service->getCharacteristic(BLEUUID((uint16_t)0x2A63)) ;
      if (cycling_power_measurement != nullptr) {
        Serial.println("  - Found Cycling Power Measurement characteristic") ;
      }
      cycling_power_measurement->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
        this->onCyclingPowerMeasurement(blerc, data, length, is_notify) ;
      }) ;    
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
