
#ifndef __LORA
#define __LORA

#include "SodaqExt_RN2483.h"

class LoRaClass : public Sodaq_RN2483
{
private:
    char framePayload[64];
    int counter;
 
public:
    LoRaClass();
    virtual ~LoRaClass() ;

	bool addBool(bool value) ;
	bool addByte(int8_t value) ;
	bool addShort(int16_t value) ;
	bool addInt(int32_t value) ;
	bool addLong(int64_t value) ;
	bool addFloat(float value) ;
	
	bool addUByte(uint8_t value);
	bool addUShort(uint16_t value);
	bool addUInt(uint32_t value);
	bool addULong(uint64_t value);		

    void flush();
    char* getFramePayload(int* len);

	bool joinOTAALoRaNetwork(uint8_t subBand,const uint8_t devEUI[8], const uint8_t appEUI[8], const uint8_t appKey[16], bool adr, uint8_t dataRate = 5);
	bool joinABPLoRaNetwork(uint8_t subBand, const uint8_t devAddr[4], const uint8_t appSKey[16], const uint8_t nwkSKey[16], bool adr, uint8_t dataRate = 5) ;
    
	int getDataRate();
    int setDataRate(uint8_t dataRate); //Added June 23rd
    int getNumberGateway();
    bool setLinkCheckCommand(uint8_t size);
	
	void hardwareReset() ;
	bool sysReset() ;
	bool sysFactoryReset() ;
	void hwInit() ;
	void swInit() ;
	
	inline void initLoRaStream(SerialType& stream) { loraStream = &stream ; }
	
protected:
} ;

extern LoRaClass LoRa ;

#endif // __LORA
