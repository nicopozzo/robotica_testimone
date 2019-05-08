#include "SD.h" //Lib to read SD card
#include "TMRpcm.h" //Lib to play auido
#include "SPI.h" //SPI lib for SD card

//=====SD and music
int SD_ChipSelectPin = 4;
TMRpcm music; //Lib object is named "music"

//=====Pins and app logic
String state;
int pressedStart = 0;
int pressedButtPins[] = {0,0,0,0};
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
int switchedState
int secondsThreshold = 60;
String request[] = {"Devo andare in bagno","Sono triste","Sono stanco","Sono contento"};




void setup() {
  state = "F0";

  for(int i=0;i<20;i++)//inizializza la coda di emozioni impostate da app. 0=prima emozione   -1=emozione non impostata
    emotionQueue[i]=-1;
    
  pinMode(startButtPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  if (!SD.begin(SD_ChipSelectPin)) { 
  Serial.println("SD fail");
  return;
  }
  
  music.setVolume(7);    //   0 to 7. Non abbiamo amplificatore e non importa se la qualità audio non è il massimo
  music.quality(1);        //  Set 1 for 2x oversampling Set 0 for normal
}



void loop() {
  String cmdIn = Serial.read();
  processCommand(cmdIn);

  //========= EVENTO BOTTONE START =============
  startButtState = digitalRead(startButtPin);
  if (startButtState == LOW && pressedStart==1) //la pressione di un bottone per quanto breve, copre più cicli. questo è per far partire una volta sola il codice
    pressedStart=0;
  if (startButtState == HIGH && pressedStart==0) {
    pressedStart=1;
    if(state == "F0"){ //stato iniziale della fun_mode. al momento se viene pigiato start allo stato F1 non succede nulla (il testimone è in attesa della risposta)
      delay(3000);// 3 secondi di tempo per permettere al ragazzo di mettersi in ascolto (bone conductor)
      emotion = getEmotion();
      music.speakerPin = boneSpeakerPin;
      music.play(emotion + ".wav");
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
        music.speakerPin = wooferPin;
        music.play(emotion + ".wav");
      }
      else{
        music.speakerPin = wooferPin;
        music.play(emotion + ".wav");
      }
      Serial.write("Seconds_" + elapsedSec);// possiamo cambiare il formato come preferite
      //TODO custom feedback (sent during the run)
      state = "T0";
    }
    
  } 

  //========= EVENTI BOTTONI EMOZIONI =============
  for(int i=0;i<4;i++){
    emotionButtState = digitalRead(emotionButtPins[i]);
    if (emotionButtState == LOW && emotionButtPins[i] == 1) { //la pressione di un bottone per quanto breve, copre più cicli. questo è per far partire una volta sola il codice
      emotionButtPins[i] = 0;
    if (emotionButtState == HIGH && emotionButtPins[i] == 0) {
      emotionButtPins[i] = 1;
      if(state == "F1"){
        if(i==emotion){
          music.speakerPin = wooferPin;
          music.play("positiveFB.wav");
          ledPositiveFB();
        }
        else{
          music.speakerPin = wooferPin;
          music.play("negativeFB.wav");
          ledNegativeFB();
        }
        state = "F0";
      }

      else if(state.substring(0,1).equals("T")){ //in train mode i bottoni servono ai ragazzi per comunicare con il terapeuta
        Serial.write(request[i]);
      }
    }
  }
  
}

void processCommand(String in){
  if((in.substring(0,8)).equals("fun_mode")){
    state = "F0";
  }
  else if((in.substring(0,8)).equals("train_mode")){
    state = "T0";
  }
  else if((in.substring(0,8)).equals("set_emotion")){
    if(state.substring(0,1).equals("F")){//solo in fun_mode
      int toAdd = in.substring(8,9).toInt();
      addEmotion(toAdd);
    }
  }
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
  analogWrite(redPin, redValue);
  analogWrite(greenPin, greenValue);
  analogWrite(bluePin, blueValue);
}

void ledNegativeFB(){
  setColor(255, 0, 0);
  delay(400)
  setColor(255, 0, 0);
  delay(400)
  setColor(255, 0, 0);
  delay(400)
  setColor(255, 0, 0);
  delay(400)
  setColor(0, 0, 0);
}

void ledPositiveFB(){
  setColor(0, 255, 0);
  delay(400)
  setColor(0, 255, 0);
  delay(400)
  setColor(0, 255, 0);
  delay(400)
  setColor(0, 255, 0);
  delay(400)
  setColor(0, 0, 0);
}
