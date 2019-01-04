//#include "IRLibAll.h" //It includes all IRLib2 functionalities (but it is less efficient and it takes up more memory space in Arduino)

#include <IRLibRecvPCI.h> //This class has the mission to receiving and managing the infrared signal.
#include <IRLibSendBase.h> //This class has the mission to sending and managing the infrared signal.
#include <IRLibDecodeBase.h> //Infrared decoder base class.
#include <IRLib_P01_NEC.h> //NEC protocol class.
#include <IRLib_P02_Sony.h> //Sony protocol class.
#include <IRLib_P07_NECx.h> //NECx protocol class.
#include <IRLibCombo.h> //Class that has mission to become the container/encapsulate the rest of protocol classes.

For this project we have used:

//HARDWARE
//-Arduino UNO
//-Infrared receiver: KY-022
//-Infrared emitter: KY-005
//-Remote control: AA59-00638A (Samsung TV)
//-Remote control: RM-SR10AV (Sony HiFi)
//-Remote control: Digital-Analog Decoder MOCHA JY-M2 QianHuan


//SOFTWARE
//-Arduino IDE 1.8.7 (Debian 8 GNU/Linux)
//-Arduino AVR Boards 1.6.21
//-IRLib2 Library (https://github.com/cyborg5/IRLib2)

//SAMSUNG TV
#define TV_PROTOCOL     NECX
#define TV_NUM_BITS     32
#define TV_POWER        0xE0E040BF
#define TV_VOLUME_UP    0xE0E0E01F
#define TV_VOLUME_DOWN  0xE0E0D02F
#define TV_VOLUME_MUTE  0XE0E0F00F

//SONY HiFi
#define HIFI_PROTOCOL   SONY
#define HIFI_NUM_BITS   12
#define HIFI_POWER      0xA81

//AUDIO DECODER MOCHA
#define AUDIO_DECODER_PROTOCOL      NEC
#define AUDIO_DECODER_BITS          32
#define AUDIO_DECODER_POWER         0x4FBC03F
#define AUDIO_DECODER_VOLUME_UP     0x4FB08F7
#define AUDIO_DECODER_VOLUME_DOWN   0x4FB10EF
#define AUDIO_DECODER_VOLUME_MUTE   0x4FB30CF

//We set that we work with IRrecvPCI object, we will work with interruptions.
IRrecvPCI myReceiver(2); //We set witch digital pin we will receive the signal or we have connected the infrared receiver sensor (in our case pin 2, because Arduino UNO accepts interruptions in this pin).
IRdecode myDecoder;   //We create an infrared decoder object.
IRsend mySender; //We create an object able to send infrared signals through the infrared emitter.

void setup() {
  Serial.begin(9600); //We set the reading speed for serial monitor.
  delay(2000);  //We wait 2 seconds
  while (!Serial); //Delay for Arduino Leonardo

  //When the TV turns on, our Arduino is powered by our TV through USB. 
  //This means that when Arduino turns on we have to send an infrared power on signal to our HiFi and audio decoder appliances.
  mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
  myReceiver.enableIRIn(); //We release/enable the interruptions.
  mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
  
  myReceiver.enableIRIn(); //We set that our infrared receiver can start listening infrared signals. We release interruptions, enable the interruptions.
  Serial.println(F("Ready to receive IR signals.")); //We show through the serial monitor a text to inform that we are ready to start.
}

void loop() {
  
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
          case TV_POWER:
            mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
            myReceiver.enableIRIn(); //We release/enable the interruptions.
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
            Serial.println("A power on/off signal has been sent to HiFi and audio decoder.");
            break;
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
        }
        
      }
      
    }
    
    //We release/enable the interruptions.
    myReceiver.enableIRIn();
    
  }
}
