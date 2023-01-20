#include "ZwiftShaper.h"
#include "ZSClient.h"
#include "ZSServer.h"

ZwiftShaper *ZS = nullptr ;
ZSClient *game = nullptr ;
ZSServer *trainer = nullptr ;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;

  ZS = new ZwiftShaper() ;
  game = ZS->getZSClient() ;
  trainer = ZS->getZSServer() ;
  
  if (game->findServer(10)){
    delay(1000) ;
    if (game->connectToServer()){
      game->setupServices() ;
      trainer->startAdvertizing() ;
      delay(300000) ;
      game->disconnectFromServer() ;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
