#ifndef BLEPROXY_H
#define BLEPROXY_H


#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <queue>


#define GEN_ACC_UUID            (uint16_t)0x1800
#define GEN_ATTR_UUID           (uint16_t)0x1801


class BLEProxyCallbacks {
  public:
    virtual std::string onRead(BLECharacteristic *c, std::string data) = 0 ;
    virtual std::string onWrite(BLECharacteristic *c, std::string data) = 0 ;
    virtual std::string onNotify(BLERemoteCharacteristic *rc, std::string data) = 0 ;
    virtual std::string onIndicate(BLERemoteCharacteristic *rc, std::string data) = 0 ;
    virtual void onTrainerDisconnect() = 0 ;
    virtual void onZwiftDisconnect() = 0 ;
} ;


class BLEProxy : public BLEServerCallbacks, public BLECharacteristicCallbacks, public BLEDescriptorCallbacks {
  private:
    std::string dev_name ;
    BLEClient *client ;
    BLEServer *server ;
    bool trainer_connected ;
    bool zwift_connected ;
    BLEAddress *trainer_address ;
    BLEAddress *zwift_address ;
    std::map<BLECharacteristic*, BLERemoteCharacteristic*> charmap ;
    std::map<BLEDescriptor*, BLERemoteDescriptor*> descmap ;
    std::queue<std::function<void()>> events ;
    BLEProxyCallbacks *callbacks ;
  
  public:
    BLEProxy(const char *name){
      dev_name = name ;
      BLEDevice::init(name) ;
      client = BLEDevice::createClient() ;
      server = BLEDevice::createServer() ;
      server->setCallbacks(this) ;
      trainer_connected = false ;
      zwift_connected = false ;
      trainer_address = nullptr ;
      zwift_address = nullptr ;
      callbacks = nullptr ;
    }


    void setCallbacks(BLEProxyCallbacks *c){
      callbacks = c ;
    }


    bool ready(){
      return (this->trainer_connected && this->zwift_connected)  ;
    }
    
    /*
      Call this method once an appripriate device has been chosen as the proxy's "server".
      This method will clone the BLE profile of the "server" into our BLEServer instance.
      
      Note: Make sure BLEScan is NOT running before calling this method.
    */
    void cloneBLEProfile(BLEAdvertisedDevice *adev){
      Serial.print("Connecting to remote device '") ;
      Serial.print(adev->getName().c_str()) ;
      Serial.println("'")  ;

      // Connect to the remote BLE Server.
      trainer_address = new BLEAddress(adev->getAddress()) ;
      Serial.print("- Trainer BLE address is ") ;
      Serial.println(trainer_address->toString().c_str()) ;
      client->connect(adev) ;
      dev_name = dev_name + std::string("[") + adev->getName() + std::string("]") ;
      ::esp_ble_gap_set_device_name(dev_name.c_str()) ;
      
      // Now we want to get all the services offered by the device, with their characteristics, 
      // and register them with our server and setup callbacks for the ones we want to change.
      std::map<std::string, BLERemoteService*> *sm = client->getServices() ;
      std::map<std::string, BLERemoteService*>::iterator it ;
      for (it = sm->begin() ; it != sm->end() ; it++){
        std::string uuid = it->first ;
        // Skip generic BLE services
        if ((uuid == BLEUUID(GEN_ACC_UUID).toString())||(uuid == BLEUUID(GEN_ATTR_UUID).toString())){
          continue ;
        }
        BLERemoteService *rs = it->second ;
        Serial.print("- Found service ") ;
        Serial.println(uuid.c_str()) ;

        // Setup this service on our server, and also for advertising
        // TODO: Advertise only the services originally advertised by the server?
        BLEService *s = server->createService(rs->getUUID()) ;
        BLEDevice::getAdvertising()->addServiceUUID(rs->getUUID()) ;
        
        std::map<std::string, BLERemoteCharacteristic*> *cm = rs->getCharacteristics() ;
        std::map<std::string, BLERemoteCharacteristic*>::iterator it ;
        for (it = cm->begin() ; it != cm->end() ; it++){
          std::string uuid = it->first ;
          BLERemoteCharacteristic *rc = it->second ;
          //Serial.print("  - Found characteristic ") ;
          //Serial.println(rc->toString().c_str()) ;
          uint32_t properties =
            (rc->canRead() ? BLECharacteristic::PROPERTY_READ : 0) |
            (rc->canWrite() ? BLECharacteristic::PROPERTY_WRITE : 0) |
            (rc->canNotify() ? BLECharacteristic::PROPERTY_NOTIFY : 0) |
            (rc->canBroadcast() ? BLECharacteristic::PROPERTY_BROADCAST : 0) |
            (rc->canIndicate() ? BLECharacteristic::PROPERTY_INDICATE : 0) |
            (rc->canWriteNoResponse() ? BLECharacteristic::PROPERTY_WRITE_NR : 0) ;
      
          // Setup this characteristic on our server
          BLECharacteristic *c = new BLECharacteristic(BLEUUID(rc->getUUID()), properties) ;
          s->addCharacteristic(c) ;
          // Copy the value from the server
          c->setValue(rc->readValue()) ;
          charmap.insert({c, rc}) ;
          c->setCallbacks(this) ;

          if (rc->canNotify()){
            // We need to set up a callback to handle these notifications
            rc->registerForNotify([=](BLERemoteCharacteristic *rc, uint8_t *data, size_t length, bool is_notify){
              if (is_notify){
                this->onNotify(rc, c, std::string(reinterpret_cast<const char *>(data), length)) ;
              }
            }, true, true) ;
          }
          if (rc->canIndicate()){
            // We need to set up a callback to handle these notifications
            rc->registerForNotify([=](BLERemoteCharacteristic *rc, uint8_t *data, size_t length, bool is_notify){
              if (! is_notify){
                this->onIndicate(rc, c, std::string(reinterpret_cast<const char *>(data), length)) ;
              }
            }, false, true) ;
          }
          
          // Descriptors
          std::map<std::string, BLERemoteDescriptor*> *dm = rc->getDescriptors() ;
          std::map<std::string, BLERemoteDescriptor*>::iterator it ;
          for (it = dm->begin() ; it != dm->end() ; it++){
            std::string uuid = it->first ;
            BLERemoteDescriptor *rd = it->second ;
            //Serial.print("  - Found descriptor ") ;
            //Serial.println(uuid.c_str()) ;

            // Setup this descriptor on our server
            BLEDescriptor *d = new BLEDescriptor(BLEUUID(rd->getUUID()), 512) ;
            c->addDescriptor(d) ;
            // Copy the value from the server
            d->setValue(rd->readValue()) ;
            descmap.insert({d, rd}) ;
            d->setCallbacks(this) ;
          }
        }

        s->start() ;
      }
    }


    void onConnect(BLEServer *srv, esp_ble_gatts_cb_param_t* param){
      // Who connected?
      BLEAddress peer(param->connect.remote_bda) ;
      if (trainer_address->equals(peer)){
        Serial.println("Trainer connected") ;
        trainer_connected = true ;
        return ;
      }
      else {
        Serial.println("Game connected") ;
        zwift_connected = true ;
        if (zwift_address == nullptr){
          zwift_address = new BLEAddress(peer) ;
        }
        return ;
      }
    }


    void onDisconnect(BLEServer *srv, esp_ble_gatts_cb_param_t* param){
      // Who disconnected?
      BLEAddress peer(param->disconnect.remote_bda) ;
      if (trainer_address->equals(peer)){
        Serial.println("Trainer disconnected") ;
        trainer_connected = false ;
        if (callbacks != nullptr){
          callbacks->onTrainerDisconnect() ;
        }
      }
      else {
        Serial.println("Game disconnected") ;        
        zwift_connected = false ;
      }
      if (callbacks != nullptr){
        callbacks->onZwiftDisconnect() ;
      }
    }


    // When Client receives a notification
    void onNotify(BLERemoteCharacteristic *rc, BLECharacteristic *c, std::string data){
      events.push([=]{
        // Serial.print("Received notification for ") ;
        // Serial.println(rc->getUUID().toString().c_str()) ;

        std::string val = data ;
        if (this->callbacks != nullptr){
          val = this->callbacks->onNotify(rc, val) ;
        }
        // Now that we had a chance to modify the data, we forward it through our server counterpart:
        c->setValue(val) ;
        c->notify() ;
        
        // Serial.println("- Forwarded") ;
      }) ;
    }


    // When Client receives an indication
    void onIndicate(BLERemoteCharacteristic *rc, BLECharacteristic *c, std::string data){
      events.push([=]{
        // Serial.print("Received indication for ") ;
        // Serial.println(rc->getUUID().toString().c_str()) ;

        std::string val = data ;
        if (this->callbacks != nullptr){
          val = this->callbacks->onIndicate(rc, val) ;
        }
        // Now that we had a chance to modify the data, we forward it through our server counterpart:
        c->setValue(val) ;
        c->indicate() ;
        
        // Serial.println("- Forwarded") ;
      }) ;
    }
    

    // When Server receives a characteristic read
    void onRead(BLECharacteristic *chr, esp_ble_gatts_cb_param_t *param){
      events.push([=]{
        Serial.print("Received read for chr ") ;
        Serial.println(chr->getUUID().toString().c_str()) ;
  
        std::string val = this->charmap[chr]->readValue() ;
        if (this->callbacks != nullptr){
          val = this->callbacks->onRead(chr, val) ;
        }
        chr->setValue(val) ;
        
        Serial.println("- Forwarded") ;
      }) ;
    }


    // When Server receives a characteristic write
    void onWrite(BLECharacteristic *chr, esp_ble_gatts_cb_param_t *param){
      bool response = param->write.need_rsp ;
      events.push([=]{
        Serial.print("Received write for chr ") ;
        if (response){
          Serial.print("(response expected) ") ;
        }
        Serial.println(chr->getUUID().toString().c_str()) ;
        
        std::string val = chr->getValue() ;
        if (this->callbacks != nullptr){
          val = this->callbacks->onWrite(chr, val) ;
        }
        this->charmap[chr]->writeValue(val, response) ;
      
        // Serial.println("- Forwarded") ;
      }) ;
    }


    // When Server receives a descriptor read
    void onRead(BLEDescriptor *d){
      events.push([=]{
        Serial.print("Received read for desc ") ;
        Serial.println(d->getUUID().toString().c_str()) ;
        
        std::string val = this->descmap[d]->readValue() ;
        d->setValue(val) ;
        
        // Serial.println("- Forwarded") ;
      }) ; 
    }    
    

    // When Server receives a descriptor write
    void onWrite(BLEDescriptor *d){
      events.push([=]{
        Serial.print("Received write for desc ") ;
        Serial.println(d->getUUID().toString().c_str()) ; 
        Serial.println(d->getCharacteristic()->toString().c_str()) ;
        
        std::string val(reinterpret_cast<const char *>(d->getValue()), d->getLength()) ;
        uint8_t *v = d->getValue() ;
        for (int i = 0 ; i < d->getLength() ; i++){
          Serial.print(v[i], HEX) ;
          Serial.print(":") ;
        }
        Serial.println() ;
        this->descmap[d]->writeValue(val) ;
      
        // Serial.println("- Forwarded") ;
      }) ;    
    }

    
    bool processEvent(){
      if (events.empty()){
        return false ;
      }
      if (! ready()){
        return false ;
      }

      std::function<void()> e = events.front() ;
      e() ;
      events.pop() ;
      
      return true ;
    }


    void printHex(std::string v){
      const char *s = v.c_str() ;
      for (int i = 0 ; i < strlen(s) ; i++){
        if (i > 0){
          Serial.print(":") ;
        }
        Serial.print(s[i], HEX) ;
      }
      Serial.println() ;      
    }
} ;


#endif
