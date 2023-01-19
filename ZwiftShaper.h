#ifndef ZWIFTSHAPER_H
#define ZWIFTSHAPER_H

#include <BLEDevice.h>
#include <BLEUtils.h>


// Supported service UUIDs, as defined in GATT specifications
#define CYCLING_POWER_SERVICE                           BLEUUID((uint16_t)0x1818) 
//#define CYCLING_SPEED_AND_CADENCE_SERVICE               BLEUUID((uint16_t)0x1816) 
//#define FITNESS_MACHINE_SERVICE                         BLEUUID((uint16_t)0x1826) 

//#define INDOOR_BIKE_CHARACTERISTIC                      BLEUUID((uint16_t)0x2AD2)
//#define FITNESS_MACHINE_CONTROL_POINT_CHARACTERISTIC    BLEUUID((uint16_t)0x2AD9)

/*
// BLE Characteristics required
BLECharacteristic INDOOR_BIKE(INDOOR_BIKE_CHARACTERISTIC, 
    BLECharacteristic::PROPERTY_NOTIFY) ;
BLECharacteristic FITNESS_MACHINE_CONTROL_POINT(FITNESS_MACHINE_CONTROL_POINT_CHARACTERISTIC, 
    BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_WRITE) ;
*/


class ZSClient : public BLEAdvertisedDeviceCallbacks {
  private:
    BLEClient *_client ;
    BLEAdvertisedDevice *_remote_device ;
    BLEUUID _remote_service_uuid ;
    BLERemoteService *_remote_service ;
    //BLERemoteCharacteristic *_indoor_bike, *_fitness_machine_control_point ;

  public:
    ZSClient(){
      _client = BLEDevice::createClient() ;
      _remote_device = nullptr ;
      _remote_service = nullptr ;
      //_indoor_bike = nullptr ;
      //_fitness_machine_control_point = nullptr ;
    }

    // Scan and locates the desired server
    BLEAdvertisedDevice *findServer(int timeout){
      Serial.println("Starting scan...") ;
      BLEScan* pBLEScan = BLEDevice::getScan() ;
      pBLEScan->setAdvertisedDeviceCallbacks(this) ;
      pBLEScan->setActiveScan(true) ;
      pBLEScan->start(timeout) ; 

      // The callbacks will set _remote_device if a proper device (.i.e. BLE Server) is found.
      // We are not yet connected to this device though.
      return _remote_device ;
    }

    // Called while scanning when a new device is discovered.
    void onResult(BLEAdvertisedDevice adev) {
      Serial.print("- Found device '") ;
      Serial.print(adev.getName().c_str()) ;
      Serial.print("': ")  ;
      Serial.println(adev.toString().c_str()) ;
      // We have found a device, let us now see if it contains the service we are looking for.
      if (adev.isAdvertisingService(CYCLING_POWER_SERVICE)){
        // For know, just pick the first one we find...
        Serial.println("  - Device with Cycling Power Service found: stopping scan") ;
        _remote_service_uuid = CYCLING_POWER_SERVICE ;
        _remote_device = new BLEAdvertisedDevice(adev) ;
        BLEDevice::getScan()->stop() ;
      }
    }

    // Connect to the BLE Server (i.e. the "trainer") at the specified address
    bool connectToServer(BLEAdvertisedDevice *dev) {
      Serial.print("Connecting to remote device '") ;
      Serial.print(dev->getName().c_str()) ;
      Serial.println("'")  ;
      // Connect to the remove BLE Server.
      _client->connect(dev) ;
                  
      // Obtain a reference to the service we are after in the remote BLE server.
      _remote_service = _client->getService(_remote_service_uuid) ;
      if (_remote_service != nullptr) {
        Serial.println("- Found remote service") ;
      } 

      if (_remote_service_uuid.equals(CYCLING_POWER_SERVICE)){
        Serial.println("CPS") ;
        setupCyclingPowerService() ;
      }

      return true ;
    }

    void disconnectFromServer(){
      Serial.print("Disconnecting to remote device") ;
      _client->disconnect() ;
    }

    void setupCyclingPowerService(){
      BLERemoteCharacteristic *cycling_power_feature_characteristic = 
        _remote_service->getCharacteristic(BLEUUID((uint16_t)0x2A65)) ;
      if (cycling_power_feature_characteristic != nullptr) {
        Serial.println("  - Found Cycling Power Feature characteristic") ;
      }     
      uint32_t cpf = cycling_power_feature_characteristic->readUInt32() ;
      Serial.print("Feature: 0b") ;
      Serial.println(cpf, BIN) ;

      BLERemoteCharacteristic *sensor_location_characteristic = 
        _remote_service->getCharacteristic(BLEUUID((uint16_t)0x2A5D)) ;
      if (sensor_location_characteristic != nullptr) {
        Serial.println("  - Found Sensor Location characteristic") ;
      }     
      uint8_t sl = sensor_location_characteristic->readUInt8() ;
      Serial.print("Location: 0x") ;
      Serial.println(sl, HEX) ;
      
      BLERemoteCharacteristic *cycling_power_measurement_characteristic = 
        _remote_service->getCharacteristic(BLEUUID((uint16_t)0x2A63)) ;
      if (cycling_power_measurement_characteristic != nullptr) {
        Serial.println("  - Found Cycling Power Measurement characteristic") ;
      }
      cycling_power_measurement_characteristic->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
        this->onCyclingPowerMeasurement(blerc, data, length, is_notify) ;
      }) ;    
    }

    void onCyclingPowerMeasurement(BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
      if (length >= 4){
        uint16_t flags = data[1] << 8 | data[0] ;
        Serial.print("Flags: 0b") ;
        Serial.println(flags, BIN) ;
        int16_t power = data[3] << 8 | data[2] ;
        Serial.print("Power: ") ;
        Serial.print(power) ;
        Serial.println("w ") ;
        if (flags & 0b10000){
          uint32_t wrevs = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4] ;
          Serial.print("Wheel Revolutions: ") ;
          Serial.println(wrevs) ;
          uint16_t lwet = data[9] << 8 | data[8] ;
        }
        if (flags & 0b100000){
          uint32_t crevs = data[11] << 8 | data[10] ;
          Serial.print("Crank Revolutions: ") ;
          Serial.println(crevs) ;
          uint16_t lwet = data[13] << 8 | data[12] ;
        }
      }
    }
    
    void setupFitnessMachineService(){
      /*
      // Obtain a reference to the characteristics in the service of the remote BLE server.
      _indoor_bike = _remote_fm_service->getCharacteristic(INDOOR_BIKE_CHARACTERISTIC) ;
      _fitness_machine_control_point = _remote_fm_service->getCharacteristic(FITNESS_MACHINE_CONTROL_POINT_CHARACTERISTIC) ;
      if (_indoor_bike != nullptr) {
          Serial.print("  - Found Indoor Bike characteristic") ;
      }
      if (_fitness_machine_control_point != nullptr) {
          Serial.print("  - Found Fitness Machine Control Point characteristic") ;
      }
      
      // Assign callback functions for the Characteristics
      // TODO: Maybe [=, this] required here
      _indoor_bike->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
          this->onIndoorBike(blerc, data, length, is_notify) ;
      }) ;
      _fitness_machine_control_point->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
          this->onFitnessMachineControlPoint(blerc, data, length, is_notify) ;
      }) ;
      */
    }

    // Called for FITNESS_MACHINE_CONTROL_POINT notifications.
    void onFitnessMachineControlPoint(BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){            
    }
} ;


#endif
