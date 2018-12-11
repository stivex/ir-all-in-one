//#include "IRLibAll.h" //Inclou tota la llibreria complerta de IRLib2 (menys eficient i ocupa més espai de memòria)

#include <IRLibRecvPCI.h> //Llibreria encarregada de rebre i gestionar la recepció de senyals infrarojos
#include <IRLibSendBase.h> //Llibreria encarregada d'enviar i gestionar l'enviament de senyals infrarojos
#include <IRLibDecodeBase.h> //Llibreria base de descodificació
#include <IRLib_P01_NEC.h> //Llibreria pel protocol NEC
#include <IRLib_P02_Sony.h> //Llibreria pel protocol Sony
#include <IRLib_P07_NECx.h> //Llibreria pel protocol NECx
#include <IRLibCombo.h> //Llibreria que s'encarregarà de fer de contenidor/encapsulador de totes les llibreries de protocols

//Per aquest projecte s'ha utilitzat:

//HARDWARE
//-Arduino UNO
//-Receptor infrarojos: KY-022
//-Emissor infrarojos: KY-005
//-Comandament a distància: AA59-00638A (TV Samsung)
//-Comandament a distància: RM-SR10AV (HIFI Sony)
//-Comandament a distància: Descodificador Digital-Analògic MOCHA JY-M2 QianHuan


//SOFTWARE
//-IDE Arduino 1.8.7 (Debian 8 GNU/Linux)
//-Arduino AVR Boards 1.6.21
//-Libreria IRLib2 (https://github.com/cyborg5/IRLib2)

//TV SAMSUNG
#define TV_PROTOCOL     NECX
#define TV_NUM_BITS     32
#define TV_POWER        0xE0E040BF
#define TV_VOLUME_UP    0xE0E0E01F
#define TV_VOLUME_DOWN  0xE0E0D02F
#define TV_VOLUME_MUTE  0XE0E0F00F

//HIFI SONY
#define HIFI_PROTOCOL   SONY
#define HIFI_NUM_BITS   12
#define HIFI_POWER      0xA81

//DESCODIFICADOR AUDIO MOCHA
#define AUDIO_DECODER_PROTOCOL      NEC
#define AUDIO_DECODER_BITS          32
#define AUDIO_DECODER_POWER         0x4FBC03F
#define AUDIO_DECODER_VOLUME_UP     0x4FB08F7
#define AUDIO_DECODER_VOLUME_DOWN   0x4FB10EF
#define AUDIO_DECODER_VOLUME_MUTE   0x4FB30CF

//Indicant que treballarem amb l'objete IRrecvPCI, treballarem amb interrupcions
IRrecvPCI myReceiver(2); //Indiquem per quin pin digital rebrem/tenim conectat sensor infrarojos (el pin 2 de l'Arduino UNO accepta interrupcions)
IRdecode myDecoder;   //Creem un objecte descodificador
IRsend mySender; //Creem un objecte per poder enviar senyals d'infraroig a través del sensor emissor 

void setup() {
  Serial.begin(9600); //Indiquem la velocitat de lectura pel monitor sèrie
  delay(2000);  //Esperem 2 segons
  while (!Serial); //delay per l'Arduino Leonardo

  //Farem que quan arrenqui l'Arduino (quan s'encengui la TV) enviarem un senyal de power a l'equip de so i el descodificador d'àudio
  mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
  myReceiver.enableIRIn(); //Alliberem
  mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
  
  myReceiver.enableIRIn(); //Indiquem en el nostre receptor que pot començar a escoltar què li arriba pel sensor d'infrarojos, habilitem les interrupcions
  Serial.println(F("Preparat per rebre senyals IR")); //Mostrem per la pantalla del monitor sèria que el programa està inicialitzat i llest
}

void loop() {
  
  //Anirem executant aquest bucle fins obenir un senyal complet

  //Aquesta funció (getResults()) mira si s'ha rebut una seqüència complerta
  if (myReceiver.getResults()) { 

    //Hem rebut un senyal complert

    //Descodofiquem el senyal complert rebut, deshabilitant les interrupcions (aquesta funció ens retorna un boleà TRUE=protocol reconegut o bé FALSE=protocol desconegut)
    if (myDecoder.decode()) {
      
      //El protocol del senyal rebut és conegut per la llibreria
      //Mostrem per pantalla el resultat de descodificar-lo. ("true" amb detall, indicar "false" perquè retorni menys detalls per pantalla)  
      myDecoder.dumpResults(false);  

      //Només tractarem els senyals provinents del comandament de la TV Samsung
      Serial.println(myDecoder.protocolNum);
      if (myDecoder.protocolNum == TV_PROTOCOL) {
        
        Serial.println("És el comandament de la TV Samsung");

        //Depenent de la tecla del comandament de la tv realizarem una acció o altre
        switch (myDecoder.value) {
          case TV_POWER:
            mySender.send(HIFI_PROTOCOL, HIFI_POWER, HIFI_NUM_BITS);
            myReceiver.enableIRIn(); //Alliberem
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_POWER, AUDIO_DECODER_BITS);
            Serial.println("S'ha enviat senyal d'encendre/apagar.");
            break;
          case TV_VOLUME_UP:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_UP, AUDIO_DECODER_BITS);
            Serial.println("S'ha enviat senyal de pujar volum.");
            break;
          case TV_VOLUME_DOWN:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_DOWN, AUDIO_DECODER_BITS);
            Serial.println("S'ha enviat senyal de baixar volum.");
            break;
          case TV_VOLUME_MUTE:
            mySender.send(AUDIO_DECODER_PROTOCOL, AUDIO_DECODER_VOLUME_MUTE, AUDIO_DECODER_BITS);
            Serial.println("S'ha enviat senyal de treure/posar volum.");
            break;          
        }
        
      }
      
    }
    
    //Reiniciem el receptor, habilitem de nou les interrupcions
    myReceiver.enableIRIn();
    
  }
}
