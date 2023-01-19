#include "ZwiftShaper.h"


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;

  ZwiftShaper ZS ;
  ZSClient *game = ZS.getZSClient() ;
  if (game->findServer(10)){
    if (game->connectToServer()){
      game->setupServices() ;
      delay(300000) ;
      game->disconnectFromServer() ;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
