//#include "IRLibAll.h" //It includes all IRLib2 functionalities (but it is less efficient and it takes up more memory space in Arduino)

#include <IRLibRecvPCI.h> //This class has the mission to receiving and managing the infrared signal.
#include <IRLibSendBase.h> //This class has the mission to sending and managing the infrared signal.
#include <IRLibDecodeBase.h> //Infrared decoder base class.
#include <IRLib_P01_NEC.h> //NEC protocol class.
#include <IRLib_P02_Sony.h> //Sony protocol class.
#include <IRLib_P07_NECx.h> //NECx protocol class.
#include <IRLibCombo.h> //Class that has mission to become the container/encapsulate the rest of protocol classes.

//For this project we have used:

//HARDWARE
//-Arduino NANO
//-Infrared receiver: KY-022
//-Infrared emitter: KY-005
//-1 LD1117AV33
//-1 buzzer (active)
//-1 red led
//-1 resistor for led: 330 ohms
//-1 photoresistor
//-1 resistor for photoresistor: 10.000 ohms
//-Remote control: AA59-00638A (Samsung TV)
//-Remote control: RM-SR10AV (Sony HiFi)
//-Remote control: Digital-Analog Decoder MOCHA JY-M2 QianHuan


//SOFTWARE
//-Arduino IDE 1.8.13 (Debian 11 GNU/Linux)
//-IRLib2 Library (https://github.com/cyborg5/IRLib2)

//SAMSUNG TV
#define TV_PROTOCOL     NECX
#define TV_NUM_BITS     32
#define TV_POWER        0xE0E040BF
#define TV_VOLUME_UP    0xE0E0E01F
#define TV_VOLUME_DOWN  0xE0E0D02F
#define TV_VOLUME_MUTE  0xE0E0F00F
#define TV_BTN_A_RED    0xE0E036C9
#define TV_BTN_B_GREEN  0xE0E028D7
#define TV_BTN_C_YELLOW 0xE0E0A857
#define TV_BTN_D_BLUE   0xE0E06897
#define TV_MENU         0xE0E058A7
#define TV_ARROW_DOWN   0xE0E08679
#define TV_ARROW_UP     0xE0E006F9
#define TV_ARROW_LEFT   0xE0E0A659
#define TV_ARROW_RIGHT  0xE0E046B9
#define TV_ENTER        0xE0E016E9
#define TV_EXIT         0xE0E0B44B
#define TV_CHANNEL_UP   0xE0E0E01F
#define TV_CHANNEL_DOWN 0xE0E008F7
#define TV_REC          0xE0E0926D



//SONY HiFi
#define HIFI_PROTOCOL     SONY
#define HIFI_NUM_BITS     12
#define HIFI_POWER        0xA81
#define HIFI_VOLUME_UP    0x481
#define HIFI_VOLUME_DOWN  0xC81

//AUDIO DECODER MOCHA
#define AUDIO_DECODER_PROTOCOL      NEC
#define AUDIO_DECODER_BITS          32
#define AUDIO_DECODER_POWER         0x4FBC03F
#define AUDIO_DECODER_VOLUME_UP     0x4FB08F7
#define AUDIO_DECODER_VOLUME_DOWN   0x4FB10EF
#define AUDIO_DECODER_VOLUME_MUTE   0x4FB30CF
#define AUDIO_DECODER_OPTICAL_1     0x4FBE01F
#define AUDIO_DECODER_OPTICAL_2     0x4FBD02F

//CONSTANTS
#define PIN_IR_SENSOR_RECEIVER 2 //pin where we have connected the infrared receiver sensor
//#define PIN_IR_SENSOR_EMITTER 3 //pin where we have connected the infrared emitter sensor
#define PIN_USB_TV 5 //pin where we have connected the USB from TV
#define PIN_BUZZER 8 //pin where we have connected the buzzer
#define PIN_LED 10 //pin where we have connected the led
#define PIN_LIGHT_SENSOR 0 //pin where we have connected the photoresistor
#define MIN_LIMIT_LIGHT 350 //the max limit of light to consider that the room is dark
#define MAX_HIFI_AUDIO 30 //the max audio that the HIFI allow
#define DELAY_IR_SIGNALS 1000 //the delay between continuous signals that it must have

//VARIABLES
int tv_status = LOW; //We save the status read on PIN_USB_TV (LOW=0=off / HIGH=1=on)
int tv_status_old = LOW; //We save the previous status that we read on PIN_USB_TV

//We set that we work with IRrecvPCI object, we will work with interruptions.
IRrecvPCI myReceiver(PIN_IR_SENSOR_RECEIVER); //We set witch digital pin we will receive the signal or we have connected the infrared receiver sensor (in our case pin 2, because Arduino UNO & NANO accepts interruptions in this pin).
IRdecode myDecoder;   //We create an infrared decoder object.
IRsend mySender; //We create an object able to send infrared signals through the infrared emitter (by default the library uses pin 3).

void setup() {
  Serial.begin(9600); //We set the reading speed for serial monitor.
  delay(2000);  //We wait 2 seconds
  while (!Serial); //Delay for Arduino Leonardo

  //We initialize digital pins (the analog pins don't need it)
  pinMode(PIN_USB_TV, INPUT); //We define the digital pin as an input
  pinMode(PIN_BUZZER, OUTPUT); //We define the digital pin as an output
  pinMode(PIN_LED, OUTPUT); //We define the digital pin as an output
  
  myReceiver.enableIRIn(); //We set that our infrared receiver can start listening infrared signals. We release interruptions, enable the interruptions.
  Serial.println(F("Ready to receive IR signals.")); //We show through the serial monitor a text to inform that we are ready to start.
}

void loop() {

  //We check if the TV is ON or OFF now
  if (!checkTvStatus()) {
    //Power status on TV has not been changed
    //In this case, we check if we are getting an IR signal from a remot control
    readIrSignals();
  }
  
}

void playSoundStart() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
  delay(100);
}

void playSoundEnd() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  }
}

void playSoundPowerOff() {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(1000);
  digitalWrite(PIN_BUZZER, LOW);
}


//We check if the TV turns ON or turns OFF (we check the power from an USB port on TV)
//If we detect the status changed from the last check we return TRUE, in other case FALSE
boolean checkTvStatus(){

  boolean status_has_changed = false;
  boolean has_room_light = false;

  tv_status = digitalRead(PIN_USB_TV); //We read the value from the PIN_USB_TV, that it's pluged on TV
  has_room_light = checkRoomLight();

    if (tv_status == HIGH && tv_status_old == LOW && has_room_light) {

      Serial.println("The TV has turned ON.");

      //We send a sound to warning us that the operation has started
      playSoundStart();

      tv_status_old = HIGH;
      status_has_changed = true;

      //We turn on the HiFi and audio decoder
      mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
      myReceiver.enableIRIn(); //We release/enable the interruptions.
      mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
      
      //Set volume HIFI at medium level
      /*
      int medium_level = (MAX_HIFI_AUDIO/2);
      for (int i = 0; i < medium_level; i++) {
        mySender.send(HIFI_PROTOCOL, HIFI_VOLUME_UP, HIFI_NUM_BITS);
        delay(DELAY_IR_SIGNALS);
      }
      */
      delay(500);
      
      //We send a sound to warning us that the operation has ended
      playSoundEnd();
    
    }
    else if (tv_status == LOW && tv_status_old == HIGH && has_room_light) {

      Serial.println("The TV has turned OFF.");

      //We send a sound to warning us that the operation has started
      playSoundStart();
      
      tv_status_old = LOW;
      status_has_changed = true;
      
      //Set optical input 1 as the default for the next time we use the TV
      mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_OPTICAL_1, AUDIO_DECODER_BITS);

      //Set volume HIFI at min
      /*
      for (int i = 0; i < MAX_HIFI_AUDIO; i++) {
        mySender.send(HIFI_PROTOCOL, HIFI_VOLUME_DOWN, HIFI_NUM_BITS);
        delay(DELAY_IR_SIGNALS);
      }
      */
      delay(500);

      //We turn off the HiFi and audio decoder
      mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
      myReceiver.enableIRIn(); //We release/enable the interruptions.
      mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);

      //We send a sound to warning us that the operation has ended
      playSoundPowerOff();
        
    } else {
      Serial.println("The power on TV has not changed or the room are dark.");
    }

  return status_has_changed;
}

//Function that it checks if the room is dark
//In case that the room has light the function returns TRUE, in other case (dark room) FALSE
boolean checkRoomLight() {
  
  int room_status = 0; //Where we save the value captured from the photoresistor
  room_status = analogRead(PIN_LIGHT_SENSOR); //We get de value
  Serial.print("The value from the photoresistor is: ");
  Serial.println(room_status);

  if(room_status < MIN_LIMIT_LIGHT) {
    //The room is dark
    digitalWrite(PIN_LED, HIGH);
    return false;
  } else {
    //The room has light
    digitalWrite(PIN_LED, LOW);
    return true;
  }
  
}

//Method that set the default configuration (audio + video) on TV
void resetDefaultsOnTv() {

  //We send a sound to warning us that the operation has started
  playSoundStart();

  //If it exists some menu that now is open, we close it
  mySender.send(TV_PROTOCOL, TV_EXIT, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We change the active source to go on TV channels
  mySender.send(TV_PROTOCOL, TV_CHANNEL_UP, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);
  delay(DELAY_IR_SIGNALS);
  delay(DELAY_IR_SIGNALS);
  mySender.send(TV_PROTOCOL, TV_CHANNEL_DOWN, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);
  delay(DELAY_IR_SIGNALS);

  //We open the menu on TV
  mySender.send(TV_PROTOCOL, TV_MENU, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);
  
  //We go down one step on the menu
  mySender.send(TV_PROTOCOL, TV_ARROW_DOWN, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We go right one step on the menu
  mySender.send(TV_PROTOCOL, TV_ARROW_RIGHT, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We go down five steps on the submenu
  for (int i = 0; i < 5; i++) {
    mySender.send(TV_PROTOCOL, TV_ARROW_DOWN, TV_NUM_BITS);
    delay(DELAY_IR_SIGNALS);
  }

  //We choose the option
  mySender.send(TV_PROTOCOL, TV_ARROW_RIGHT, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We choose the option
  mySender.send(TV_PROTOCOL, TV_ARROW_RIGHT, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We go down one step on the menu
  mySender.send(TV_PROTOCOL, TV_ARROW_DOWN, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We choose the option
  mySender.send(TV_PROTOCOL, TV_ENTER, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We close all menus
  mySender.send(TV_PROTOCOL, TV_EXIT, TV_NUM_BITS);
  delay(DELAY_IR_SIGNALS);

  //We send a sound to warning us that the operation has ended
  playSoundEnd();
  
}

//Method that reads signals from TV remote control
void readIrSignals() {

  //We execute this bucle method until getting a complete IR signal.

  //This function getResults() checks if we have received a complete IR sequence.
  if (myReceiver.getResults()) { 

    //We have received a complete IR sequence.

    //We decode the IR signal that we have received, we disable the interruptions (this function returns TRUE when it recognises the protocol, in other case FALSE).
    if (myDecoder.decode()) {
      
      //The protocol has been recognised by the library.
      //We show through the serial monitor the result of decoding the signal (TRUE with details, FALSE without details).
      myDecoder.dumpResults(false);  

      //We only analyze the signal that comes from our TV.
      Serial.println(myDecoder.protocolNum);
      if (myDecoder.protocolNum == TV_PROTOCOL) {
        
        Serial.println("The signal comes from TV remote control.");

        //It depends which TV remote control key we have pressed, we will do an action or another.
        switch (myDecoder.value) {
          case TV_VOLUME_UP:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_UP, AUDIO_DECODER_BITS);
            Serial.println("A volume up signal has been sent to audio decoder.");
            break;
          case TV_VOLUME_DOWN:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_DOWN, AUDIO_DECODER_BITS);
            Serial.println("A volume down signal has been sent to audio decoder.");
            break;
          case TV_VOLUME_MUTE:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_MUTE, AUDIO_DECODER_BITS);
            Serial.println("A mute volume signal has been sent to audio decoder.");
            break;
          case TV_BTN_A_RED:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_OPTICAL_1, AUDIO_DECODER_BITS);
            Serial.println("An optical input 1 signal has been sent to audio decoder.");
            break;
          case TV_BTN_B_GREEN:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_OPTICAL_2, AUDIO_DECODER_BITS);
            Serial.println("An optical input 2 signal has been sent to audio decoder.");
            break;
          case TV_BTN_C_YELLOW:
            mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
            myReceiver.enableIRIn(); //We release/enable the interruptions.
            Serial.println("A power on/off signal has been sent to HiFi.");
            break;
          case TV_BTN_D_BLUE:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
            myReceiver.enableIRIn(); //We release/enable the interruptions.
            Serial.println("A power on/off signal has been sent to audio decoder.");
            break;
          case TV_REC:
            resetDefaultsOnTv();
            Serial.println("A reset audio and video configuration has been sent on TV.");
            break;
        }
        
      }
      
    }
    
    //We release/enable the interruptions.
    myReceiver.enableIRIn();
    
  }
  
}
