#ifndef BLEPROXY_H
#define BLEPROXY_H


#include <BLEDevice.h>
#include <BLEUtils.h>


#define GEN_ACC_UUID            (uint16_t)0x1800
#define GEN_ATTR_UUID           (uint16_t)0x1801


class BLEProxy : public BLEServerCallbacks {
  private:
    std::string dev_name ;
    BLEClient *client ;
    BLEServer *server ;
    bool server_connected ;
    bool client_connected ;

  public:
    BLEProxy(const char *name){
      dev_name = name ;
      BLEDevice::init(name) ;
      ble_client = BLEDevice::createClient() ;
      ble_server = BLEDevice::createServer() ;
      ble_server->setCallbacks(this) ;
      server_connected = false ;
      client_connected = false ;
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
      dev_name = dev_name + std::string("[") + getName() + std::string("]") ;

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
          BLERemoteCharacteristic *remchr = it->second ;
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
          c->setCallbacks(shaper->getZSServer()) ;
          s->addCharacteristic(c) ;
          // TODO: Descriptors
          // indoorBike.addDescriptor(new BLE2902()) ;

          if (rc->canNotify()){
            // We need to set up a callback to handle these notifications
            rc->registerForNotify([=](BLERemoteCharacteristic* blerc, uint8_t* data, size_t length, bool is_notify){
              if ((! this->server_connected)||(! this->client_connected)){
                return ;
              }

              Serial.print("Received notification for ") ;
              Serial.println(blerc->getUUID().toString().c_str()) ;

              // Now that we had a chance to modify the data, we forward it through our server counterpart:
              c->setValue(data, length) ;
              c->notify() ;

              Serial.print("- Forwarded ") ;
            }) ;
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
        Serial.println("Client connection") ;
        client_connected = true ;
        return ;
      }
    }
    
    void onDisconnect(BLEServer *srv){
      Serial.println("Disconnect") ;
    }
} ;


#endif
