#include "ZwiftShaper.h"


ZSServer::ZSServer(ZwiftShaper *zs, BLEServer *ble_server, BLEClient *ble_client){
  shaper = zs ;
  server = ble_server ;
  client = ble_client ;
  client_connected = false ;

  BLEAdvertising *adv = BLEDevice::getAdvertising() ;
  adv->setScanResponse(true) ;
  adv->setMinPreferred(0x06) ;
}


void ZSServer::startAdvertizing(){
  std::string name = std::string("ZS[") + shaper->getName() + std::string("]") ;
  ::esp_ble_gap_set_device_name(name.c_str()) ;
  server->setCallbacks(this) ;
  BLEDevice::startAdvertising() ;
}


// Callback called on client connect
void ZSServer::onConnect(BLEServer *srv) {
  Serial.println("Connected") ;
  client_connected = true ;
}


// Callback called on client disconnect
void ZSServer::onDisconnect(BLEServer *srv) {
  Serial.println("Disconnected") ;
  client_connected = false ;
  // server->setCallbacks(nullptr) ;
  // startAdvertizing() ;
}


void ZSServer::onRead(BLECharacteristic *chr){
}


void ZSServer::onWrite(BLECharacteristic *chr){
}


bool ZSServer::isClientConnected(){
  return client_connected ;
}
