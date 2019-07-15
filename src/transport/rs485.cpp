//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "rs485.h"
#include "../utils/crc8.h"
#include "../config.h"
#include <stddef.h>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RS485::RS485(Stream& s, uint8_t _dePin,uint32_t tmout)
{
	dePin = _dePin;
	workStream = &s;
	writePtr = 0;
	rsPacketPtr = (uint8_t*) &rs485Packet;
	dataBuffer = NULL;
	receivedDataLength = 0;
	receivedData = NULL;
	
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RS485::~RS485()
{
	delete [] dataBuffer;
	wipe();
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::begin()
{
	DBGLN(F("RS485: begin."));
	Pin::mode(dePin,OUTPUT);
	switchToReceive();
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::switchToSend()
{
	Pin::write(dePin,HIGH); // переводим контроллер RS-485 на передачу
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::switchToReceive()
{    
	Pin::write(dePin,LOW); // переводим контроллер RS-485 на приём
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RS485::write(const uint8_t* data, uint16_t dataLength)
{
	DBGLN(F("RS485: send data..."));

	RS485Packet outPacket;
	outPacket.dataLength = dataLength;
	outPacket.dataCrc = crc8(data,dataLength);

	uint8_t* p = (uint8_t*)&outPacket;
	outPacket.packetCrc = crc8(p,sizeof(RS485Packet) - 1);

	switchToSend();

	workStream->write(p,sizeof(RS485Packet));
	workStream->write(data,dataLength);

	waitTransmitComplete();

	switchToReceive();

	DBGLN(F("RS485: data was sent."));

	return true;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::waitTransmitComplete()
{
	workStream->flush(); 
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::update()
{
	while(workStream->available())
	{
	  rsPacketPtr[writePtr++] = (uint8_t) workStream->read();
		   
	  if(gotRS485Packet())
	  {
		if(processRS485Packet())
		{
			// получили пакет, копируем данные пакета к себе
			receivedDataLength = rs485Packet.dataLength;
			delete [] receivedData;
			receivedData = dataBuffer;
			dataBuffer = NULL;
		}
	  }
	} // while 
  
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RS485::gotRS485Packet()
{
  // проверяем, есть ли у нас полный RS-485 пакет
  if(writePtr > ( sizeof(RS485Packet)-1 ))
  {
      // вычитали N байт из потока, надо проверить - не пакет ли это?
      if(!(rs485Packet.stx1 == STX1 && rs485Packet.stx2 == STX2))
      {
         // заголовок неправильный, ищем возможное начало пакета
         uint8_t readPtr = 0;
         bool startPacketFound = false;
         while(readPtr < sizeof(RS485Packet))
         {
           if(rsPacketPtr[readPtr] == STX1)
           {
            startPacketFound = true;
            break;
           }
            readPtr++;
         } // while
    
         if(!startPacketFound) // не нашли начало пакета
         {
            writePtr = 0; // сбрасываем указатель чтения и выходим
            return false;
         }
    
         if(readPtr == 0)
         {
          // стартовый байт заголовка найден, но он в нулевой позиции, следовательно - что-то пошло не так
            writePtr = 0; // сбрасываем указатель чтения и выходим
            return false;       
         } // if
    
         // начало пакета найдено, копируем всё, что после него, перемещая в начало буфера
         
         uint8_t thisWritePtr = 0;
         uint8_t bytesWritten = 0;
         while(readPtr < sizeof(RS485Packet) )
         {
          rsPacketPtr[thisWritePtr++] = rsPacketPtr[readPtr++];
          bytesWritten++;
         }

    
         writePtr = bytesWritten; // запоминаем, куда писать следующий байт

         return false;
             
      } // if
      else
      {
        // заголовок совпал, проверяем жопку
        if(!(rs485Packet.etx1 == ETX1 && rs485Packet.etx2 == ETX2))
        {
          // окончание неправильное, сбрасываем указатель чтения и выходим
          writePtr = 0;
          return false;
        }
  
        // жопка совпала, проверяем CRC пакета
        // данные мы получили, сразу обнуляем указатель записи, чтобы не забыть
        writePtr = 0;
    
        // проверяем контрольную сумму
        uint8_t crc = crc8(rsPacketPtr,sizeof(RS485Packet) - 1);
        if(crc != rs485Packet.packetCrc)
        {
          // не сошлось, игнорируем
          DBGLN(F("RS485: BAD PACKET CRC!!!"));
          return false;
        }
        
        // CRC сошлось, пакет валидный
        return true;              
      } // else
  } // if
  
  return false;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RS485::processRS485Packet()
{
   bool receiveResult = false;
   
    // у нас в пакете лежит длина данных, надо их вычитать из потока
    DBG(F("RS485: DATA TO READ: "));
    DBGLN(rs485Packet.dataLength);

    uint16_t readed = 0;
    delete [] dataBuffer;
    dataBuffer = new uint8_t[rs485Packet.dataLength];
    memset(dataBuffer,0,rs485Packet.dataLength);
    
    uint32_t startReadingTime = uptime();
    bool hasTimeout = false;
    
    while(readed < rs485Packet.dataLength)
    {
      while(workStream->available() && readed < rs485Packet.dataLength)
      {
        dataBuffer[readed++] = workStream->read();
        startReadingTime = uptime();
      }

      if(uptime() - startReadingTime > receiveTimeout) // таймаут чтения
      {
        DBGLN(F("RS485: RECEIVE TIMEOUT!!!"));
        hasTimeout = true;
        break;
      }
    } // while

    bool isCrcGood = false;
    
    if(rs485Packet.dataLength)
    {
        uint8_t dataCrc = crc8(dataBuffer,rs485Packet.dataLength);
        
        if(dataCrc == rs485Packet.dataCrc)
        {
          isCrcGood = true;
          DBG(F("RS485: DATA RECEIVED = "));
          #ifdef _DEBUG
            DEBUG_SERIAL.write(dataBuffer,rs485Packet.dataLength);
            DEBUG_SERIAL.println();
          #endif        
        }
        else
        {
          DBGLN(F("RS485: BAD DATA CRC!!!"));
        }
    } // if(rs485Packet.dataLength)
    else
    {
      isCrcGood = true; // нет данных в пакете 
    }

   receiveResult = isCrcGood && !hasTimeout;
        
  return receiveResult;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RS485::available()
{
	return (receivedDataLength > 0 && receivedData);
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t* RS485::read(uint16_t& readed)
{	
	readed = receivedDataLength;
	return receivedData;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RS485::wipe()
{
	receivedDataLength = 0;
	delete [] receivedData;
	receivedData = NULL;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
