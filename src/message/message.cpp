//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "message.h"
#include "../config.h"
#include "../data/anydata.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message::Message()
{
	payload = NULL;
	payloadLength = 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message::Message(uint32_t _controllerID, uint8_t _moduleID, Messages _type)
{
	payload = NULL;
	payloadLength = 0;
	controllerID = _controllerID;
	moduleID = _moduleID;
	type = _type;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message::~Message()
{
	delete [] payload;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message::Message(const Message& rhs)
{
	delete [] payload;
	payloadLength = rhs.payloadLength;
	payload = new uint8_t[payloadLength];
	memcpy(payload,rhs.payload,payloadLength);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message& Message::operator=(const Message& rhs)
{
	delete [] payload;
	payloadLength = rhs.payloadLength;
	payload = new uint8_t[payloadLength];
	memcpy(payload,rhs.payload,payloadLength);
	
	return *this;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::parse(const uint8_t* rawData, uint16_t rawDataLength)
{
	Message m;

	// если сообщение слишком маленькое - это ошибка !!!
	if(rawDataLength < MESSAGE_HEADER_SIZE)
		return m;
	
	// копируем сырое сообщение к себе
	m.payloadLength = rawDataLength;	
	m.payload = new uint8_t[m.payloadLength];
	memcpy(m.payload,rawData,m.payloadLength);
	
	
	// теперь из сырого сообщения копируем в переменные основную информацию
	
	// ID контроллера
	uint8_t* ptr = (uint8_t*)&m.controllerID;
	
	for(size_t i=0;i<sizeof(uint32_t);i++)
	{
		*ptr++ = *rawData++;
	}
	
	// ID модуля
	ptr = (uint8_t*)&m.moduleID;
	*ptr = *rawData++;
	
	// тип сообщения
	uint16_t t;
	ptr = (uint8_t*)&t;
	
	for(size_t i=0;i<sizeof(uint16_t);i++)
	{
		*ptr++ = *rawData++;
	}
	
	m.type = static_cast<Messages>(t);
	
	
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t* Message::writeHeader(uint8_t* raw, uint32_t controllerID, uint8_t moduleID, uint16_t type)
{
	memcpy(raw,&controllerID,sizeof(uint32_t));
	raw += sizeof(uint32_t);

	memcpy(raw,&moduleID,sizeof(uint8_t));
	raw += sizeof(uint8_t);
	
	memcpy(raw,&type,sizeof(uint16_t));
	raw += sizeof(uint16_t);
	
	return raw;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::ScanResponse(uint32_t controllerID, uint8_t moduleID, const char* moduleName, uint8_t broadcastDataCount,uint8_t observeDataCount)
{
	/*
	отсылается дочерним модулем в ответ на сообщение "сканирую эфир", структура:
	
		ID контроллера
		ID модуля
		Тип сообщения - "я на связи"
		нагрузка:
			- длина имени модуля (1 байт)
			- символьное имя модуля
			- кол-во исходящих слотов виртуальных данных, которые публикует модуль (1 байт)
			- кол-во входящих слотов виртуальных данных, которые слушает модуль (1 байт)	
	*/	
	
	Message m(controllerID,moduleID,Messages::ScanResponse);
		
	// конструируем сырое сообщение
	uint8_t nameLen = strlen(moduleName);
	m.payloadLength = MESSAGE_HEADER_SIZE + nameLen + 3;
	m.payload = new uint8_t[m.payloadLength];	
	uint8_t* writePtr = Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	// пишем полезную нагрузку
	memcpy(writePtr,&nameLen,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);
	
	memcpy(writePtr,moduleName,nameLen);
	writePtr += nameLen;

	memcpy(writePtr,&broadcastDataCount,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);

	memcpy(writePtr,&observeDataCount,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::Pong(uint32_t controllerID, uint8_t moduleID)
{
	/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "понг"
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		отсылается модулем на контроллер как ответ на сообщение "пинг", структура:

		ID контроллера
		ID модуля
		Тип сообщения - "понг"
	*/
	
	Message m(controllerID,moduleID,Messages::Pong);
	
	// конструируем сырое сообщение
	m.payloadLength = MESSAGE_HEADER_SIZE;
	m.payload = new uint8_t[m.payloadLength];
	Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::BroadcastSlotData(uint32_t controllerID, uint8_t moduleID, AnyData* dt)
{
	/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные исходящего слота"
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем в ответ на запрос "регистрация исходящего слота", структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "данные исходящего слота"
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				- тип данных слота (температура и т.п., 2 байта)
				- флаги (наличие данных и пр., 1 байт)
				- длина данных слота (2 байта)
				- данные слота
	*/
	
	Message m(controllerID,moduleID,Messages::BroadcastSlotData);
	
	// конструируем сырое сообщение
	uint16_t dataLen = dt->getDataLength();
	
	m.payloadLength = MESSAGE_HEADER_SIZE + dataLen + 7;
	m.payload = new uint8_t[m.payloadLength];
	uint8_t* writePtr = Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	// копируем нагрузку
	uint16_t helper16 = dt->getID();
	memcpy(writePtr,&helper16,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);
	
	helper16 = static_cast<uint16_t>(dt->getType());
	memcpy(writePtr,&helper16,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);

	uint8_t helper8 = dt->hasData();
	memcpy(writePtr,&helper8,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);
	
	memcpy(writePtr,&dataLen,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);

	memcpy(writePtr,dt->getData(),dataLen);
	writePtr += dataLen;
	
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::ObserveSlotData(uint32_t controllerID, uint8_t moduleID, uint16_t slotID, uint32_t frequency)
{
	/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные входящего слота"
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем как ответ на запрос "регистрация входящего слота", структура:
		
		ID контроллера
		ID модуля
		Тип сообщения - "данные входящего слота"
		нагрузка:
			- ID слота (уникальный в рамках системы ID слота, 2 байта)
			- период публикации контроллером данных слота в эфир, миллисекунд (4 байта)	
	*/	
	
	Message m(controllerID,moduleID,Messages::ObserveSlotData);
	
	m.payloadLength = MESSAGE_HEADER_SIZE + 6;
	m.payload = new uint8_t[m.payloadLength];
	uint8_t* writePtr = Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	
	// копируем нагрузку
	memcpy(writePtr,&slotID,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);	

	memcpy(writePtr,&frequency,sizeof(uint32_t));
	writePtr += sizeof(uint32_t);	
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::AnyDataResponse(uint32_t controllerID, uint8_t moduleID, AnyData* dt)
{
	/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "ответ данных слота"
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем как ответ на сообщение "запрос данных слота", структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "ответ данных слота"
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				- тип данных слота (температура и т.п., 2 байта)
				- флаги (наличие данных и пр., 1 байт)
				- длина данных слота (2 байта)
				- данные слота
	
	*/
	
	Message m(controllerID,moduleID,Messages::AnyDataResponse);
	
	// конструируем сырое сообщение
	uint16_t dataLen = dt->getDataLength();
	
	m.payloadLength = MESSAGE_HEADER_SIZE + dataLen + 7;
	m.payload = new uint8_t[m.payloadLength];
	uint8_t* writePtr = Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	// копируем нагрузку
	uint16_t helper16 = dt->getID();
	memcpy(writePtr,&helper16,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);
	
	helper16 = static_cast<uint16_t>(dt->getType());
	memcpy(writePtr,&helper16,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);

	uint8_t helper8 = dt->hasData();
	memcpy(writePtr,&helper8,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);
	
	memcpy(writePtr,&dataLen,sizeof(uint16_t));
	writePtr += sizeof(uint16_t);

	memcpy(writePtr,dt->getData(),dataLen);
	writePtr += dataLen;
	
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
