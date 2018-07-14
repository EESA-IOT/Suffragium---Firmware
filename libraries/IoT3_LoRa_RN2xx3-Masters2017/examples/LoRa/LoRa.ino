/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

#define DEBUG
#include <LoRa.h>
#define debugSerial SerialUSB
#define loraSerial  Serial2

// === CUSTOM DEFINITION ===
#define LORA_PORT     2

// For OTAA provisioning, use the internal IEEE HWEUI provided 
// by Microchip module RN2483 module as the devEUI key
// (comment to not use)
// #define USE_INTERNAL_HWEUI

// OTAA Keys - Use your own KEYS!

const uint8_t devEUI[8]  = {0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x} ;
const uint8_t appEUI[8]  = {0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x} ;  
const uint8_t appKey[16] = {0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x}; 


// Payload is 'Microchip ExpLoRer' in HEX
const uint8_t myPayload[] = {0x4D, 0x69, 0x63, 0x72, 0x6F, 0x63, 0x68, 0x69, 0x70, 0x20, 0x45, 0x78, 0x70, 0x4C, 0x6F, 0x52, 0x65, 0x72} ;

// Reception buffer
uint8_t frameReceived[255] ;

void setup()
{
  while ((!debugSerial) && (millis() < 10000)) ;
	debugSerial.begin(115200) ;
  debugSerial.println("Microchip Technology ExpLoRer Starter Kit") ;

  initLed() ;
  initButton() ;
  initTemperature() ;

  // LoRa Init procedure
  LoRa.setDiag(debugSerial) ; // optional
  LoRa.hwInit() ;
	loraSerial.begin(LoRa.getDefaultBaudRate()) ;
  LoRa.initLoRaStream(loraSerial) ;
  LoRa.swInit() ;

  // == Get the internal Hardware EUI of the module ==
	uint8_t hwEUI[8] ;
  uint8_t len = LoRa.getHWEUI(hwEUI, sizeof(hwEUI)) ;
  debugSerial.println("") ;
  if (len > 0)
  {
    debugSerial.print("Internal Hardware EUI = ") ;
    char c[2] ;
    // put in form for displaying in hex format
    for (uint8_t i = 0; i < len; i++)
    {
      sprintf(c, "%02X", hwEUI[i]) ;
      debugSerial.print(c) ;
    }
    debugSerial.println("") ;

    #ifdef USE_INTERNAL_HWEUI
      memcpy(devEUI, hwEUI, sizeof(hwEUI)) ;
    #endif    
  }
  else
  {
    debugSerial.println("Failed to get the internal hardware EUI") ;
  }

  // == Join the network ==
  debugSerial.println("Try to join the network with the following keys ...") ;
  debugSerial.print("devEUI = ") ;
  displayArrayInOneLine(devEUI, sizeof(devEUI)) ;
  debugSerial.println("") ;
  debugSerial.print("appEUI = ") ;
  displayArrayInOneLine(appEUI, sizeof(appEUI)) ;
  debugSerial.println("") ;  
  debugSerial.print("appKey = ") ;
  displayArrayInOneLine(appKey, sizeof(appKey)) ;  
  debugSerial.println("") ;
    
  bool res = 0;
  int tentative = 0;
  do
  {
    setRgbColor(0x00, 0x00, 0xFF) ;
    debugSerial.println("OTAA Join Request") ;
    res = LoRa.joinLoRaNetwork(devEUI, appEUI, appKey, true, 5) ;
    debugSerial.println(res ? "OTAA Join Accepted." : "OTAA Join Failed! Trying again. Waiting 10 seconds.") ;
    if (!res)
    {
      setRgbColor(0xFF, 0x00, 0x00) ;
      tentative++ ;
      delay(10000) ;
    }
    if (tentative == 3) 
    {
      while(1) 
      {
        setRgbColor(0xFF, 0x00, 0x00) ;
        delay(1000);
        setRgbColor(0x00, 0x99, 0xFF) ;
        delay(1000);
        setRgbColor(0xFF, 0xFF, 0xFF) ;
        delay(1000);
      }
    }
  } while (res == 0) ;

  debugSerial.println("Sleeping for 5 seconds before starting.") ;
  sleep(5) ;

  debugSerial.println("Push the button to send a message over the LoRa Network ...") ;
}

void loop()
{
  bool pushButton = false ;
  
  if (getButton() == LOW)
  {
    // Button has been pressed
    turnBlueLedOn() ;
    pushButton = true ;
    while(getButton() == LOW) ;
    // Button has been released
    turnBlueLedOff() ;
    pushButton = false ;

    debugSerial.println("Try to transmit an uplink message ...") ;
    switch (LoRa.send(LORA_PORT, myPayload, sizeof(myPayload)))
    {
      case NoError:
        debugSerial.println("Successful transmission.") ;
        digitalWrite(LED_BUILTIN, LOW) ;
        setRgbColor(0x00, 0xFF, 0x00) ;
        break;
      case NoResponse:
        debugSerial.println("There was no response from the device.") ;
        setRgbColor(0xFF, 0x00, 0x00) ;
        break ;
      case Timeout:
        debugSerial.println("Connection timed-out. Check your serial connection to the device! Sleeping for 20sec.") ;
        setRgbColor(0xFF, 0x00, 0x00) ;
        break ;
      case PayloadSizeError:
        debugSerial.println("The size of the payload is greater than allowed. Transmission failed!") ;
        setRgbColor(0xFF, 0x00, 0x00) ;
        break ;
      case InternalError:
        debugSerial.println("Oh No! This shouldn't happen. Something is really wrong! Try restarting the device!\r\nThe program will now halt.") ;
        setRgbColor(0xFF, 0x00, 0x00) ;
        while (1) {} ;
        break ;
      case Busy:
        debugSerial.println("The device is busy. Sleeping for 10 extra seconds.");
        delay(10000) ;
        break ;
      case NetworkFatalError:
        debugSerial.println("There is a non-recoverable error with the network connection. You should re-connect.\r\nThe program will now halt.");
        setRgbColor(0xFF, 0x00, 0x00) ;
        while (1) {} ;
        break ;
      case NotConnected:
        debugSerial.println("The device is not connected to the network. Please connect to the network before attempting to send data.\r\nThe program will now halt.");
        setRgbColor(0xFF, 0x00, 0x00) ;
        while (1) {} ;
        break ;
      case NoAcknowledgment:
        debugSerial.println("There was no acknowledgment sent back!");
        setRgbColor(0xFF, 0x00, 0x00) ;
        break ;
      default:
        break ;
    }

    // Downlink
    int frameReceivedSize = LoRa.receive(frameReceived, sizeof(frameReceived)) ;
    debugSerial.print("Downlink received : ") ;
    for (int i = 0; i<frameReceivedSize; i++)
    {
      debugSerial.print(frameReceived[i], HEX);
      debugSerial.print(" ");
    }
    debugSerial.println("");
    debugSerial.println("Sleep for 60 seconds") ;
    sleep(60) ;
    debugSerial.println("Push the button to send a message over the LoRa Network ...") ;
  } 
}

void displayArrayInOneLine(const uint8_t tab[], uint8_t tabSize)
{
  for (uint8_t i = 0; i < tabSize; i++)
  {
    debugSerial.print(tab[i], HEX) ;
  }
}

void sleep(uint8_t t)
{
  for (uint8_t i = t; i > 0; i--)
  {
    debugSerial.println(i) ;
    delay(1000) ;
  }  
}

// --- Temperature sensor
void initTemperature()
{
  pinMode(TEMP_SENSOR, INPUT) ;
  //Set ADC resolution to 12 bits
  analogReadResolution(12) ;  
}

float getTemperature()
{
  float mVolts = (float)analogRead(TEMP_SENSOR) * 3300.0 / 1023.0 ;
  float temp = (mVolts - 500.0) / 100.0 ;
  return temp ;
}

// --- RGB LED and BLUE LED
void initLed()
{
  pinMode(LED_BUILTIN, OUTPUT) ;
  pinMode(LED_RED, OUTPUT) ;
  pinMode(LED_GREEN, OUTPUT) ;
  pinMode(LED_BLUE, OUTPUT) ;  
}

#define COMMON_ANODE  // LED driving
void setRgbColor(uint8_t red, uint8_t green, uint8_t blue)
{
#ifdef COMMON_ANODE
  red = 255 - red ;
  green = 255 - green ;
  blue = 255 - blue ;
#endif
  analogWrite(LED_RED, red) ;
  analogWrite(LED_GREEN, green) ;
  analogWrite(LED_BLUE, blue) ;
}

void turnBlueLedOn()
{
  digitalWrite(LED_BUILTIN, HIGH) ;
}

void turnBlueLedOff()
{
  digitalWrite(LED_BUILTIN, LOW) ;
}

// --- Push Button
void initButton()
{
  pinMode(BUTTON, INPUT_PULLUP) ;  
}

bool getButton()
{
  return digitalRead(BUTTON) ;
}
