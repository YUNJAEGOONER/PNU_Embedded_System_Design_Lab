#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

/*
  0001.mp3 : 수통에 물이 부족합니다. 물을 채워주세요.
  0002.mp3 : 토양에 수분이 부족합니다. 
  0003.mp3 : 식물이 정상적으로 관리되고 있습니다.
  0004.mp3 : 식물과 너무 가까이 있습니다.
*/

/* Configure Pin Map */
// inner ESP32
#define SONAR_TRIG 25
#define SONAR_ECHO 26
#define WATER_LEVEL 33

// interface with Player Module
#define RXD2 16
#define TXD2 17
#define PLAYER_BUSY 14

// interface with STM32
#define IS_CLOSE 19 // from sonar module to stm32
#define IS_ENOUGH 18  // from water level module to stm32

#define AUTO_MODE 22 // from stm32 to esp32
#define SOIL_MOISTURE 4 // from stm32 to esp32


/*Setting Limit Value */
#define LEVEL_LIMIT 50
#define DISTANCE_LIMIT 20

/* Define Sensor Data Structure */
typedef struct _SensorData { int distance; int level; } SensorData;
typedef struct _SensorState { bool isClose; bool isEnough; bool isBusy;} SensorState;
typedef struct _Stm32State { bool mode; bool wetSoil; } Stm32State;

HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;

SensorData sensorData = {0,0};
SensorState sensorState = {false, false, true};
Stm32State stm32State = {false, false};

void readDistance() {
  bool prevClose = sensorState.isClose;
  
  digitalWrite(SONAR_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(SONAR_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(SONAR_TRIG, LOW);
  sensorData.distance = pulseIn(SONAR_ECHO, HIGH) / 58;
  sensorState.isClose = sensorData.distance < DISTANCE_LIMIT;
  delay(100);  
  digitalWrite(IS_CLOSE, sensorState.isClose);
  // 수동 모드일 때는 음성을 출력하지 않음 
  if (sensorState.isClose && (!prevClose)) {
      playInformation();
  }
}

// 1 물통 물 2 토양 수분 3 식물 정상 4 너무 가까움
void playInformation() {
  Serial.println("Play..");
  sensorState.isBusy = digitalRead(PLAYER_BUSY);
  if (sensorState.isBusy) {
    if (!sensorState.isEnough) {
        myDFPlayer.play(3); // 
    } else if (!stm32State.wetSoil) {
        myDFPlayer.play(2); // 
    } else {
        myDFPlayer.play(1); // 
    }
  }
}

void readWaterLevel() {
  sensorData.level = analogRead(WATER_LEVEL);
  sensorState.isEnough = sensorData.level > LEVEL_LIMIT;
  digitalWrite(IS_ENOUGH, sensorState.isEnough);
}

void readFromStm32() {
  stm32State.mode = digitalRead(AUTO_MODE);
  stm32State.wetSoil = digitalRead(SOIL_MOISTURE);
  Serial.print("Soil : ");
  Serial.println(stm32State.wetSoil);
}

void setup()
{
  mySoftwareSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.begin(115200);
  myDFPlayer.begin(mySoftwareSerial);
  // sensor
  pinMode(SONAR_TRIG, OUTPUT);
  pinMode(SONAR_ECHO, INPUT);
  pinMode(WATER_LEVEL, INPUT);

  // interface
  // stm32 -> esp32
  pinMode(SOIL_MOISTURE, INPUT);
  pinMode(AUTO_MODE, INPUT);

  // esp32 -> stm32
  pinMode(IS_CLOSE, OUTPUT);
  pinMode(IS_ENOUGH, OUTPUT);

  // player -> esp32
  pinMode(PLAYER_BUSY, INPUT);

  // initialize
  sensorState.isClose = true;
  sensorState.isEnough = true;
  stm32State.wetSoil = digitalRead(SOIL_MOISTURE);
  myDFPlayer.volume(25);

}

void loop()
{
  readWaterLevel();
  delay(200);
  readFromStm32();
  delay(200);
  readDistance();
  delay(200);
}