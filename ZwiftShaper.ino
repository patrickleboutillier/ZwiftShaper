#include "ZwiftShaper.h"


ZwiftShaper ZS ;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;

  ZSClient *game = ZS.getZSClient() ;
  if (game->findServer(10)){
    if (game->connectToServer()){
      delay(300000) ;
      game->disconnectFromServer() ;
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
