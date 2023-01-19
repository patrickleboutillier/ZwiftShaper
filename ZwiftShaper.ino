#include "ZwiftShaper.h"


void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600) ;

    BLEDevice::init("");
    ZSClient *game = new ZSClient() ;
    BLEAdvertisedDevice *trainer = game->findServer(10) ;
    game->connectToServer(trainer) ;
    delay(300000) ;
    game->disconnectFromServer() ;
}

void loop() {
  // put your main code here, to run repeatedly:

}
