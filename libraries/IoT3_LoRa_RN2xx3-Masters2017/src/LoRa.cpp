
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "LoRa.h"
#include "SodaqExt_RN2483.h"
#include "StringLiterals.h"
#include "Utils.h"
#include "Sodaq_wdt.h"

#define STR_DR					"dr "

#define CHECK_COUNTER(X)		if((counter + (X - 1)) >= 64) return false;  // X = byte length of value

LoRaClass LoRa;

LoRaClass::LoRaClass() : Sodaq_RN2483()
{
    counter = 0;
    memset(framePayload, 0, 64);
}

LoRaClass::~LoRaClass()
{
}

void LoRaClass::flush()
{
	counter = 0;
	memset(framePayload, 0, 64);
}

bool LoRaClass::addBool(bool value)
{
    CHECK_COUNTER(1);    
    framePayload[counter++] = value;
    return true;
}

bool LoRaClass::addByte(int8_t value)
{
    CHECK_COUNTER(1);    
    framePayload[counter++] = value;
    return true;
}

bool LoRaClass::addShort(int16_t value)
{
    CHECK_COUNTER(2);

    framePayload[counter++] = value >> 8;
    framePayload[counter++] = value & 0xFF;
	
    return true;
}

bool LoRaClass::addInt(int32_t value)
{
    CHECK_COUNTER(4);

    framePayload[counter++] = value >> 24;
    framePayload[counter++] = (value >> 16) & 0xFF;;
    framePayload[counter++] = (value >> 8) & 0xFF;
    framePayload[counter++] = value & 0xFF;
    return true;
}
bool LoRaClass::addLong(int64_t value)
{
    CHECK_COUNTER(8);

	framePayload[counter++] = value >> 56;
    framePayload[counter++] = (value >> 48) & 0xFF;
	framePayload[counter++] = (value >> 40) & 0xFF;
    framePayload[counter++] = (value >> 32) & 0xFF;
    framePayload[counter++] = (value >> 24) & 0xFF;
    framePayload[counter++] = (value >> 16) & 0xFF;;
    framePayload[counter++] = (value >> 8) & 0xFF;
    framePayload[counter++] = value & 0xFF;
    return true;
}

bool LoRaClass::addFloat(float value)
{
	CHECK_COUNTER(4);
	char floatValue[4];
	memcpy(&floatValue, &value, 4);
	framePayload[counter++] = floatValue[3];
	framePayload[counter++] = floatValue[2];
	framePayload[counter++] = floatValue[1];
	framePayload[counter++] = floatValue[0];
	return true;
}

bool LoRaClass::addUByte(uint8_t value)
{
    CHECK_COUNTER(1);    
    framePayload[counter++] = value;
    return true;
}

bool LoRaClass::addUShort(uint16_t value)
{
    CHECK_COUNTER(2);

    framePayload[counter++] = value >> 8;
    framePayload[counter++] = value & 0xFF;
	
    return true;
}

bool LoRaClass::addUInt(uint32_t value)
{
    CHECK_COUNTER(4);

    framePayload[counter++] = value >> 24;
    framePayload[counter++] = (value >> 16) & 0xFF;;
    framePayload[counter++] = (value >> 8) & 0xFF;
    framePayload[counter++] = value & 0xFF;
    return true;
}

bool LoRaClass::addULong(uint64_t value)
{
    CHECK_COUNTER(8);

	framePayload[counter++] = value >> 56;
    framePayload[counter++] = (value >> 48) & 0xFF;;
	framePayload[counter++] = (value >> 40) & 0xFF;
    framePayload[counter++] = (value >> 32) & 0xFF;
    framePayload[counter++] = (value >> 24) & 0xFF;
    framePayload[counter++] = (value >> 16) & 0xFF;;
    framePayload[counter++] = (value >> 8) & 0xFF;
    framePayload[counter++] = value & 0xFF;
    return true;
}

char* LoRaClass::getFramePayload(int* len)
{
    if(len == 0) return NULL;
    *len = counter;
    return framePayload;
}


bool LoRaClass::joinOTAALoRaNetwork(uint8_t subBand, const uint8_t devEUI[8], const uint8_t appEUI[8], const uint8_t appKey[16], bool adr, uint8_t dataRate)
{    
	uint8_t dataRateBuffer[1] = {dataRate};
    
    return resetDevice() &&		setFsbChannels(subBand) &&
		setMacParam(STR_DR, dataRateBuffer, 1) &&
        setMacParam(STR_DEV_EUI, devEUI, 8) &&
        setMacParam(STR_APP_EUI, appEUI, 8) &&
        setMacParam(STR_APP_KEY, appKey, 16) &&
        setMacParam(STR_ADR, BOOL_TO_ONOFF(adr)) &&
        joinNetwork(STR_OTAA);
}

bool LoRaClass::joinABPLoRaNetwork(uint8_t subBand, const uint8_t devAddr[4], const uint8_t appSKey[16], const uint8_t nwkSKey[16], bool adr, uint8_t dataRate)
{    
	uint8_t dataRateBuffer[1] = {dataRate};
	this->diagStream->println("[initOTA]"); //Luc

    return resetDevice() &&		setFsbChannels(subBand) &&
		setMacParam(STR_DR, dataRateBuffer, 1) &&
        setMacParam(STR_DEV_ADDR, devAddr, 4) &&
        setMacParam(STR_APP_SESSION_KEY, appSKey, 16) &&
        setMacParam(STR_NETWORK_SESSION_KEY, nwkSKey, 16) &&
        setMacParam(STR_ADR, BOOL_TO_ONOFF(adr)) &&
        joinNetwork(STR_ABP);
}

//Added June 23rd
int LoRaClass::setDataRate(uint8_t dataRate)
{
    return setMacParam(STR_DR, &dataRate, 1);
}


int LoRaClass::getDataRate()
{
    this->loraStream->print(STR_CMD_GET_DATA_RATE);
	this->loraStream->print(CRLF);
    
    unsigned long start = millis();
    while (millis() < start + DEFAULT_TIMEOUT) {
        sodaq_wdt_reset();

        if (readLn() > 0) {
			return atoi(this->inputBuffer);         
        }
    }

    return -1;
}

int LoRaClass::getNumberGateway()
{
	this->loraStream->print(STR_CMD_GET_NUMBER_GATEWAY);
	this->loraStream->print(CRLF);
  
	int numberGateway = -1;
    unsigned long start = millis();
    while (millis() < start + DEFAULT_TIMEOUT) {
        sodaq_wdt_reset();

        if (readLn() > 0) {
			return atoi(this->inputBuffer);         
        }
    }    
  
	return -1;
}

void LoRaClass::hardwareReset()
{
	pinMode(LORA_RESET, OUTPUT) ;
	digitalWrite(LORA_RESET, LOW) ;
	delay(1) ;
	digitalWrite(LORA_RESET, HIGH) ;
	delay(500) ;	
}

void LoRaClass::hwInit()
{
	hardwareReset() ;
}

bool LoRaClass::sysReset()
{
	return (resetDevice()) ;
}

bool LoRaClass::sysFactoryReset()
{
    this->loraStream->print(STR_CMD_FACTORY_RESET) ;
    this->loraStream->print(CRLF) ;

    if (expectString(STR_DEVICE_TYPE_RN, 10000)) {
        if (strstr(this->inputBuffer, STR_DEVICE_TYPE_RN2483) != NULL) {
            return true;
        }
        else if (strstr(this->inputBuffer, STR_DEVICE_TYPE_RN2903) != NULL) {
            // TODO move into init once it is decided how to handle RN2903-specific operations
            setFsbChannels(DEFAULT_FSB);
            return true;
        }
        else {
            return false;
        }
    }
    return false;	
}

void LoRaClass::swInit()
{
	sysFactoryReset() ;
	sysReset() ;
}
