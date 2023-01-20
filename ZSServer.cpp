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


// Callback called on client connect.
// Strangely enough, this also gets triggered when the BLEClient establishes a connection, so 
// to disable that the setCallbacks() method is called only after the BLEClient has connected with the trainer
// and the service setup is finished.
void ZSServer::onConnect(BLEServer *srv) {
  Serial.println("Connection received by ZSServer") ;
  client_connected = true ;
}


// Callback called on client disconnect
// Strangely enough, this also gets triggered when the BLEClient receives a disconnection
void ZSServer::onDisconnect(BLEServer *srv) {
  if (client_connected){
    Serial.println("Disconnection received by ZSServer") ;
    client_connected = false ;
    // Disconnect client
    client->disconnect() ;
  }
  else {
    Serial.println("Disconnection received by ZSClient") ;
    // Disconnect server
    disconnect() ;
  }

  // For now, in both cases the best thing to do here is to reset the unit and start over...
  ESP.restart() ;
}


// Transfer read over to the client and store it?
void ZSServer::onRead(BLECharacteristic *chr){
    Serial.print("Received read for chr ") ;
    Serial.println(chr->getUUID().toString().c_str()) ;
    BLERemoteService *remsrvc = client->getService(chr->getService()->getUUID()) ;
    BLERemoteCharacteristic *remchr = remsrvc->getCharacteristic(chr->getUUID()) ;
    std::string val = remchr->readValue() ;
    chr->setValue(val) ;
}


void ZSServer::onWrite(BLECharacteristic *chr){
    Serial.print("Received write for chr ") ;
    Serial.println(chr->getUUID().toString().c_str()) ;
}


bool ZSServer::isClientConnected(){
  return client_connected ;
}
