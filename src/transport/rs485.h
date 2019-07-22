#pragma once
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "transport.h"
#include <Arduino.h>
#include "../config.h"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct RS485Packet
{
  uint8_t stx1;
  uint8_t stx2;
  uint16_t dataLength;
  uint8_t dataCrc;
  uint8_t etx1;
  uint8_t etx2;
  uint8_t packetCrc;

  RS485Packet()
  {
    stx1 =  STX1;
    stx2 =  STX2;

    etx1 =  ETX1;
    etx2 =  ETX2;

    dataLength = 0;
    dataCrc = 0;
    packetCrc = 0;   
  }
  
};
#pragma pack(pop)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class RS485 : public Transport
{
	public:
	
		RS485(Stream& s, uint8_t dePin,uint32_t tmout);
		~RS485();
	
		void begin();
		bool write(const uint8_t* payload, uint16_t payloadLength);
		uint8_t* read(uint16_t& readed);
		bool available();
		void wipe();
		void update();
		uint32_t getReadingTimeout() { return receiveTimeout; }
		
		
private:

    void waitTransmitComplete();
    bool gotRS485Packet();
    bool processRS485Packet();
    void switchToSend();
    void switchToReceive();

 
    RS485Packet getDataReceived(uint8_t* &data);

	
    uint8_t dePin;
    Stream* workStream;

    uint8_t writePtr;
    RS485Packet rs485Packet;
    uint8_t* rsPacketPtr;
    uint32_t receiveTimeout;
   
    uint8_t* dataBuffer;
	
	uint16_t receivedDataLength;
	uint8_t* receivedData;

		
};

