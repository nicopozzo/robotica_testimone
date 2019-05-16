#include <SoftwareSerial.h>
#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


//===== Bluetooth 
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TO_BE_MIMED_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define MIMING_CHARACTERISTIC_UUID "e7380755-7e83-499c-8de2-7c8169452fac"
#define SET_MODE_CHARACTERISTIC_UUID "f2afd722-713e-4e66-8f7a-6630c3ebc931"
BLECharacteristic *p_to_be_mimed_Characteristic;
BLECharacteristic *p_miming_Characteristic;
BLECharacteristic *p_set_mode_Characteristic;

//===== music players

SoftwareSerial mySoftwareSerial_bone(22,23); // RX, TX
DFRobotDFPlayerMini myDFPlayer_bone;

SoftwareSerial mySoftwareSerial_woofer(20,21); // RX, TX
DFRobotDFPlayerMini myDFPlayer_woofer;

//=====Pins and app logic
String state;
int pressedStart = 0;
int pressedButtPins[] = {0,0,0,0};
int startPressing = 0; //variabile che conta quanto tempo resta pigiato il tasto start. 5 secondi ---> train mode
const int wooferPin = 2;
const int boneSpeakerPin = 2;
const int startButtPin = 2;
const int emotionButtPins[] = {2,2,2,2};
const int redPin= 2;
const int greenPin = 2;
const int bluePin = 2;


//======Fun mode
int emotionQueue[20]; // emozioni impostate dall'app per la fun mode
int startButtState, emotion, emotionButtState;

//======Train mode
int startTime, endTime, elapsedSec;
int switchedState;
int secondsThreshold = 60;
String request[] = {"Devo andare in bagno","Sono triste","Sono stanco","Sono contento"};


void initializePlayers(){
  mySoftwareSerial_bone.begin(9600);
  if (!myDFPlayer_bone.begin(mySoftwareSerial_bone)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to start Mp3Player_bone:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  myDFPlayer_bone.volume(30);  //Set volume value. From 0 to 30

  mySoftwareSerial_woofer.begin(9600);
  if (!myDFPlayer_woofer.begin(mySoftwareSerial_woofer)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to start Mp3Player_woofer:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  myDFPlayer_woofer.volume(30);  //Set volume value. From 0 to 30
}

void BLE_setup(){
  Serial.println("Starting BLE work!");

  BLEDevice::init("EPS32 testimone delle emozioni");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  p_to_be_mimed_Characteristic = pService->createCharacteristic(
                                         TO_BE_MIMED_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  p_miming_Characteristic = pService->createCharacteristic(
                                         MIMING_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ //read only
                                       );
  p_set_mode_Characteristic = pService->createCharacteristic(
                                         SET_MODE_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       
  p_miming_Characteristic->setValue("def");
  p_to_be_mimed_Characteristic->setValue("def");
  p_set_mode_Characteristic->setValue("def");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE setup complete");
}

void setup() {
  Serial.begin(115200);
  initializePlayers();
  BLE_setup();

  
  state = "F0";

  for(int i=0;i<20;i++)//inizializza la coda di emozioni impostate da app. 0=prima emozione   -1=emozione non impostata
    emotionQueue[i]=-1;
    
  pinMode(startButtPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void addEmotion(int emotion){
  int inserted = 0;
  for(int i=0;i<10;i++)
  {
    if(emotionQueue[i]==-1){
      emotionQueue[i] = emotion;
      inserted = 1;
    }
  }
  if(inserted==0){
    Serial.write("Coda piena. Impossibile impostare più di 20 emozioni");
  }
}

void BLE_read(){

  //==== SET_MODE
  std::string cmdIn = p_set_mode_Characteristic->getValue();
  if(cmdIn.compare("fun_mode")){
    state = "F0";
    p_set_mode_Characteristic->setValue("def");
  }
  else if(cmdIn.compare("train_mode")){
    state = "T0";
    p_set_mode_Characteristic->setValue("def");
  }

  //==== TO_BE_MIMED
  cmdIn = p_to_be_mimed_Characteristic->getValue();
  if(!cmdIn.compare("def")){
    if(state.substring(0,1).equals("F")){//solo in fun_mode
      int toAdd = std::atoi(cmdIn.c_str());//cmdIn.toInt();
      addEmotion(toAdd);
      p_to_be_mimed_Characteristic->setValue("def");
    }
  }
  
}


int getEmotion(){
  int toret = emotionQueue[0];
  if(toret==-1){ //se non ci sono emozioni impostate da app
    toret = random(4);
  }
  else{
    for(int i=0;i<9;i++)//coda FIFO quindi scorre
    {
      emotionQueue[i]==emotionQueue[i+1];
    }
    emotionQueue[9]=-1;
  }
  return toret;
}

void setColor(int redValue, int greenValue, int blueValue) {
  //analogWrite(redPin, redValue);   DA FIXARE, EPS NON SUPPORTA ANALOGWRITE
  //analogWrite(greenPin, greenValue);
  //analogWrite(bluePin, blueValue);
}

void ledNegativeFB(){
  setColor(255, 0, 0);
  delay(400);
  setColor(255, 0, 0);
  delay(400);
  setColor(255, 0, 0);
  delay(400);
  setColor(255, 0, 0);
  delay(400);
  setColor(0, 0, 0);
}

void ledPositiveFB(){
  setColor(0, 255, 0);
  delay(400);
  setColor(0, 255, 0);
  delay(400);
  setColor(0, 255, 0);
  delay(400);
  setColor(0, 255, 0);
  delay(400);
  setColor(0, 0, 0);
}

void loop() {
  BLE_read();
  

  //========= EVENTO BOTTONE START =============
  startButtState = digitalRead(startButtPin);
  if (startButtState == LOW && pressedStart==1){ //la pressione di un bottone per quanto breve, copre più cicli. questo è per far partire una volta sola il codice
    pressedStart=0;
    startPressing=0;
  }
  if (startButtState == HIGH) {
    if(startPressing == 0)
      startPressing = millis();
    else
      if((millis()-startPressing)/1000 >= 5){
        state = "T0";
        Serial.write("Modalità allenamento impostata");
      }
    if(pressedStart==0){
      pressedStart=1;
      if(state == "F0"){ //stato iniziale della fun_mode. al momento se viene pigiato start allo stato F1 non succede nulla (il testimone è in attesa della risposta)
        emotion = getEmotion();
        p_miming_Characteristic->setValue(emotion);
        delay(3000);// 3 secondi di tempo per permettere al ragazzo di mettersi in ascolto (bone conductor)
        myDFPlayer_bone.play(emotion); //emotion [0,3]
        state = "F1";
      }
  
      switchedState = 0;
      if(state.equals("T0")){
        startTime = millis();
        state = "T1";
        switchedState = 1;
      }
      
      if(state.equals("T1") && switchedState == 0){
        endTime = millis();
        elapsedSec = (endTime-startTime)/1000;
        if(elapsedSec < secondsThreshold){
          myDFPlayer_woofer.play(emotion);
        }
        else{
          myDFPlayer_woofer.play(emotion);
        }
        Serial.write("Seconds_" + elapsedSec);// possiamo cambiare il formato come preferite
        //TODO custom feedback (sent during the run)
        state = "T0";
      }
    }
  } 

  //========= EVENTI BOTTONI EMOZIONI =============
  for(int i=0;i<4;i++){
    emotionButtState = digitalRead(emotionButtPins[i]);
    if (emotionButtState == LOW && pressedButtPins[i] == 1) { //la pressione di un bottone per quanto breve, copre più cicli. questo è per far partire una volta sola il codice
      pressedButtPins[i] = 0;
    if (emotionButtState == HIGH && pressedButtPins[i] == 0) {
      pressedButtPins[i] = 1;
      if(state == "F1"){
        if(i==emotion){
          myDFPlayer_woofer.play(0);
          ledPositiveFB();
        }
        else{
          myDFPlayer_woofer.play(1);
          ledNegativeFB();
        }
        state = "F0";
      }

      else if(state.substring(0,1).equals("T")){ //in train mode i bottoni servono ai ragazzi per comunicare con il terapeuta
        //Serial.write(request[i]); TODO BT
      }
    }
  }
  delay(100);
}
}









//fun mode:
//- cell -> eps : emozione da far mimare         TO_BE_MIMED
//- eps -> cell : emozione che sta venendo mimata    MIMING
//- cell -> eps : selezione modalità              SET_MODE
//train mode:
//- eps -> cell : necessità del ragazzo    NEEDS
//- eps -> cell : tempo di corsa    RUN_TIME
//- cell -> eps : custom feedback TTS se facciamo in tempo ad implementarlo   CUST_FEEDBACK
//- cell -> eps : distanza da correre     RUN_DISTANCE


//files su sd_bone: 
//0.mp3   Emozione 1
//1.mp3   Emozione 2
//2.mp3   Emozione 3
//3.mp3   Emozione 4
//files su sd_woofer:
//0.mp3   Feedback positivo (TODO metterne più di uno e riprodurli a random)
//1.mp3   Feedback negativo (TODO metterne più di uno e riprodurli a random)
