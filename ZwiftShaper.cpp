#include "ZwiftShaper.h"

#include "ZSClient.h"
#include "ZSServer.h"


ZwiftShaper::ZwiftShaper(){
  name = "ZS[]" ;
  BLEDevice::init(name.c_str()) ;
  ble_client = BLEDevice::createClient() ;
  ble_server = BLEDevice::createServer() ;
  zs_client = new ZSClient(this, ble_client, ble_server) ;
  zs_server = new ZSServer(this, ble_server, ble_client) ;
}

ZSClient *ZwiftShaper::getZSClient(){
  return zs_client ;
}

ZSServer *ZwiftShaper::getZSServer(){
  return zs_server ;
}

BLEServer *ZwiftShaper::getBLEServer(){
  return ble_server ;
}

BLEClient *ZwiftShaper::getBLEClient(){
  return ble_client ;
}

std::string ZwiftShaper::getName(){
  return name ;
}

void ZwiftShaper::setName(std::string n){
  name = n ;
}
