#ifndef BLEPROXY_H
#define BLEPROXY_H


#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>


#define GEN_ACC_UUID            (uint16_t)0x1800
#define GEN_ATTR_UUID           (uint16_t)0x1801


class BLEProxyCallbacks {
  public:
    virtual std::string onRead(BLECharacteristic *c, std::string data) = 0 ;
    virtual std::string onNotify(BLERemoteCharacteristic *rc, std::string data) = 0 ;
    virtual void onDisconnect(bool server_connected, bool client_connected) = 0 ;
} ;


class BLEProxy : public BLEServerCallbacks, public BLECharacteristicCallbacks, public BLEDescriptorCallbacks {
  private:
    std::string dev_name ;
    BLEClient *client ;
    BLEServer *server ;
    bool server_connected ;
    bool client_connected ;
    std::map<BLECharacteristic*, BLERemoteCharacteristic*> chrmap ;
    std::vector<std::function<void()>> events ;
    BLEProxyCallbacks *callbacks ;
  
  public:
    BLEProxy(const char *name){
      dev_name = name ;
      BLEDevice::init(name) ;
      client = BLEDevice::createClient() ;
      server = BLEDevice::createServer() ;
      server->setCallbacks(this) ;
      server_connected = false ;
      client_connected = false ;
      callbacks = nullptr ;
    }


    void setCallbacks(BLEProxyCallbacks *c){
      callbacks = c ;
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
          Serial.print("  - Found characteristic ") ;
          Serial.println(rc->toString().c_str()) ;
          uint32_t properties =
            (rc->canRead() ? BLECharacteristic::PROPERTY_READ : 0) |
            (rc->canWrite() ? BLECharacteristic::PROPERTY_WRITE : 0) |
            (rc->canNotify() ? BLECharacteristic::PROPERTY_NOTIFY : 0) |
            (rc->canBroadcast() ? BLECharacteristic::PROPERTY_BROADCAST : 0) |
            (rc->canIndicate() ? BLECharacteristic::PROPERTY_INDICATE : 0) |
            (rc->canWriteNoResponse() ? BLECharacteristic::PROPERTY_WRITE_NR : 0) ;
      
          // Setup this characteristic on our server
          BLECharacteristic *c = new BLECharacteristic(BLEUUID(rc->getUUID()), properties) ;
          c->setCallbacks(this) ;
          s->addCharacteristic(c) ;
          chrmap.insert({c, rc}) ;

          if (rc->canNotify()){
            // We need to set up a callback to handle these notifications
            rc->registerForNotify([=](BLERemoteCharacteristic *rc, uint8_t *data, size_t length, bool is_notify){
              if (is_notify){
                this->onNotify(rc, c, std::string(reinterpret_cast<const char *>(data), length)) ;
              }
            }) ;
          }

          // Descriptors
          std::map<std::string, BLERemoteDescriptor*> *dm = rc->getDescriptors() ;
          std::map<std::string, BLERemoteDescriptor*>::iterator it ;
          for (it = dm->begin() ; it != dm->end() ; it++){
            std::string uuid = it->first ;
            BLERemoteDescriptor *rd = it->second ;
            Serial.print("  - Found descriptor ") ;
            Serial.println(uuid.c_str()) ;

            // Setup this descriptor on our server
            BLEDescriptor *d = new BLEDescriptor(BLEUUID(rd->getUUID()), 512) ;
            d->setCallbacks(this) ;
            c->addDescriptor(d) ;
          }
        }

        s->start() ;
      }
    }

    
    void onConnect(BLEServer *srv){
      if (! server_connected){
        Serial.println("Server connected") ;
        server_connected = true ;
        return ;
      }
      if (! client_connected){
        Serial.println("Client connected") ;
        client_connected = true ;
        return ;
      }
    }


    void onDisconnect(BLEServer *srv){
      Serial.println("Server or client disconnected") ;
      if (callbacks != nullptr){
        callbacks->onDisconnect(server_connected, client_connected) ;
      }
    }


    // When Client receives a notification
    void onNotify(BLERemoteCharacteristic *rc, BLECharacteristic *c, std::string data){
      events.push_back([=]{
        //Serial.print("Received notification for ") ;
        //Serial.println(rc->getUUID().toString().c_str()) ;

        std::string mdata = data ;
        if (this->callbacks != nullptr){
          mdata = this->callbacks->onNotify(rc, mdata) ;
        }
        
        // Now that we had a chance to modify the data, we forward it through our server counterpart:
        c->setValue(mdata) ;
        c->notify() ;
        
        //Serial.println("- Forwarded") ;
      }) ;
    }


    // When Server receives a characteristic read
    void onRead(BLECharacteristic *chr, esp_ble_gatts_cb_param_t *param){
      events.push_back([=]{
        Serial.print("Received read for chr ") ;
        Serial.println(chr->getUUID().toString().c_str()) ;
  
        std::string val = this->chrmap[chr]->readValue() ;
        chr->setValue(val) ;
        
        Serial.println("- Forwarded") ;
      }) ;
    }


    // When Server receives a characteristic write
    void onWrite(BLECharacteristic *chr, esp_ble_gatts_cb_param_t *param){
      bool response = param->write.need_rsp ;
      events.push_back([=]{
        Serial.print("Received write for chr ") ;
        Serial.println(chr->getUUID().toString().c_str()) ;
        
        std::string val = chr->getValue() ;
        this->chrmap[chr]->writeValue(val, response) ;
      
        Serial.println("- Forwarded") ;
      }) ;
    }


    // When Server receives a descriptor read
    void onRead(BLEDescriptor *d){
      Serial.print("Received read for desc ") ;
      Serial.println(d->getUUID().toString().c_str()) ;
    }    
    

    // When Server receives a descriptor write
    void onWrite(BLEDescriptor *d){
      Serial.print("Received write for desc ") ;
      Serial.println(d->getUUID().toString().c_str()) ;
    }

    
    void processEvents(){
      if (events.size() == 0){
        return ;
      }
      if ((! this->server_connected)||(! this->client_connected)){
        return ;
      }

      //Serial.print("Processing ") ;
      //Serial.print(events.size()) ;
      //Serial.println(" events") ;
      std::vector<std::function<void()>>::iterator it ;
      for (it = events.begin() ; it != events.end() ; it++){
        (*it)() ;
      }
      events.clear() ;
    }
} ;


#endif
