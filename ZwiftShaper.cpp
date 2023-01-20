#include "ZwiftShaper.h"

#include "ZSClient.h"
#include "ZSServer.h"


ZwiftShaper::ZwiftShaper(){
  name = "ZS[]" ;
  BLEDevice::init(name.c_str()) ;
  client = new ZSClient(this) ;
  server = new ZSServer(this) ;
}

ZSClient *ZwiftShaper::getZSClient(){
  return client ;
}

ZSServer *ZwiftShaper::getZSServer(){
  return server ;
}

BLEServer *ZwiftShaper::getBLEServer(){
  return server->getServer() ;
}

std::string ZwiftShaper::getName(){
  return name ;
}

void ZwiftShaper::setName(std::string n){
  name = n ;
}
