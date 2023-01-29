#include "ZwiftShaper.h"
#include <PinButton.h>

#define LED_BUILTIN 2
#define GREEN       23
#define RED         22
#define YELLOW      21
#define POWER       36
PinButton BUTTON(39, INPUT) ;

#define POWER_MODE_OFF        0
#define POWER_MODE_NOT_BELOW  1
#define POWER_MODE_NOT_ABOVE  2
#define POWER_MODE_ON_TARGET  3

#define MAX_POWER   250

#define GRADE_MODE_OFF        0
#define GRADE_MODE_SUPPRESS   1

bool reconnect_trainer = false ;
bool reconnect_zwift = false ;


class MyZwiftShaperCallbacks : public ZwiftShaperCallbacks {
  private:
    uint16_t trainer_power, trainer_cadence, target_power, effective_power ;
    float game_grade, target_grade, effective_grade ;
    uint8_t power_mode, grade_mode ;
    
  public:
    MyZwiftShaperCallbacks(){
        trainer_power = 0 ;
        trainer_cadence = 0 ;
        target_power = 0 ;
        effective_power = 0 ;
        game_grade = 0 ;
        target_grade = 0 ;
        effective_grade = 0 ;
        power_mode = 0 ;
        grade_mode = 0 ;
    }
    
    uint8_t setNextPowerMode(){
      switch(power_mode){
        case 0: power_mode = 1 ; break ;
        case 1: power_mode = 3 ; break ;
        case 2: power_mode = 0 ; break ;
        case 3: power_mode = 2 ; break ;
      }
      return power_mode ;
    }

    uint8_t setNextGradeMode(){
      grade_mode = !grade_mode ;
      return grade_mode ;
    }

    // Called when trainer sends power value to game
    uint16_t onPower(uint16_t watts){
      trainer_power = watts ;
      target_power = map(analogRead(POWER), 0, 4095, 0, MAX_POWER) ;
      
      switch (power_mode){
        case POWER_MODE_OFF:
          effective_power = trainer_power ;
          break ;
        case POWER_MODE_NOT_BELOW:
          effective_power = (trainer_power < target_power ? target_power : trainer_power) ;          
          break ;
        case POWER_MODE_NOT_ABOVE:
          effective_power = (trainer_power > target_power ? target_power : trainer_power) ;
          break ;
        case POWER_MODE_ON_TARGET:
          effective_power = target_power ;
          break ;
      }
      
      return effective_power ;
    }

    // Called when game sends grade value to trainer
    float onGrade(float grade){
      game_grade = grade ;
      effective_grade = (grade_mode == GRADE_MODE_SUPPRESS ? 0 : game_grade) ;
      return effective_grade ;
    }

    // Called when trainer sends cadence data to game
    void onCadence(uint16_t rpms){
      trainer_cadence = rpms ;
    }

    void status(){
      char buf[256] ;
      sprintf(buf, "POWER[IN:%3d, TARGET:%3d, OUT:%3d]  CADENCE[IN:%3d]  GRADE[IN:%5.2lf, OUT:%5.2lf]\n", 
        trainer_power, target_power, effective_power,
        trainer_cadence,
        game_grade, effective_grade
        ) ;
      Serial.print(buf) ;
    }

    void onTrainerDisconnect(){
      ESP.restart() ;
    }

    void onZwiftDisconnect(){
      reconnect_zwift = true ;
    }
} ;


ZwiftShaper *ZS = nullptr ;
BLEProxy *PROXY = nullptr ;
MyZwiftShaperCallbacks *MZSC = new MyZwiftShaperCallbacks() ;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200) ;
  pinMode(LED_BUILTIN, OUTPUT) ;
  pinMode(GREEN, OUTPUT) ;
  pinMode(YELLOW, OUTPUT) ;
  pinMode(RED, OUTPUT) ;
  pinMode(POWER, INPUT) ;

  ZS = new ZwiftShaper() ;
  ZS->setCallbacks(MZSC) ;
  PROXY = new BLEProxy("ZS") ;
  PROXY->setCallbacks(ZS) ;
  
  Serial.println("Starting scan...") ;
  BLEScan *scan = BLEDevice::getScan() ;
  scan->setAdvertisedDeviceCallbacks(ZS) ;
  scan->setActiveScan(true) ;
  scan->start(15) ;

  BLEAdvertisedDevice *rd = ZS->getRemoteDevice() ;
  if (rd != nullptr){
    PROXY->cloneBLEProfile(rd) ;
    Serial.println("Starting advertisement...") ;
    BLEDevice::startAdvertising() ;
  }
}


unsigned long then = millis() ;

void loop() {
  // Keep this as short as possible...
  if (PROXY->processEvent()){
    return ;
  }

  BUTTON.update() ;
  if (BUTTON.isSingleClick()){
    uint8_t pm = MZSC->setNextPowerMode() ;
    digitalWrite(RED, pm & 0b10) ;
    digitalWrite(GREEN, pm & 0b01) ;
    return ;
  }
  else if (BUTTON.isLongClick()){
    uint8_t gm = MZSC->setNextGradeMode() ;
    digitalWrite(YELLOW, gm) ;
    return ;
  }

  if (PROXY->ready()){
    unsigned long now = millis() ;
    if ((now - then) > 1000){
      then = now ;
      MZSC->status() ;
    }
  }
  else {
    if (reconnect_zwift){
      Serial.println("Starting advertisement...") ;
      BLEDevice::startAdvertising() ;
      reconnect_zwift = false ;
    }
  }
}
