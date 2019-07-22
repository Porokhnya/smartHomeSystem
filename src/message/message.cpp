//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "message.h"
#include "../config.h"
#include "../data/anydata.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern "C" {
static void __noregistration(bool b){}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void registration(bool result) __attribute__ ((weak, alias("__noregistration")));
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
Message Message::Scan(uint32_t controllerID, uint8_t moduleID)
{
/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "сканирую эфир" (Scan)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		посылается при начале работы контроллером, предназначено для поиска зарегистрированных модулей. 
		Модуль при этом должен сброситься на начало работы своей логики (под вопросом).
		
		структура:
		
			ID контроллера
			ID модуля, к которому направлен запрос
			Тип сообщения - "сканирую эфир" (Scan)
	
	
		как только модуль принял сообщение, и оно адресовано ему - он отсылает через транспорт, которым получено сообщение, сообщение вида "я на связи" (ScanResponse)
*/	

	Message m(controllerID,moduleID,Messages::Scan);
	
	// конструируем сырое сообщение
	m.payloadLength = MESSAGE_HEADER_SIZE;
	m.payload = new uint8_t[m.payloadLength];
	Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	
	return m;
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
Message Message::RegistrationResult(uint32_t controllerID, uint8_t moduleID)
{
	/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "регистрация завершена" (RegistrationResult)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается модулем в ответ на сообщение "запрос регистрации" (RegistrationRequest), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "регистрация завершена" (RegistrationResult)
	*/
	
	Message m(controllerID,moduleID,Messages::RegistrationResult);
	
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
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Message Message::EventResponse(uint32_t controllerID, uint8_t moduleID, uint8_t hasEvent, Event* e)
{
/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "публикация события" (EventResponse)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		отсылается модулем в ответ на сообщение "запрос события" (EventRequest), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "публикация события" (EventResponse)
			нагрузка:
				- Флаг, есть событие или нет (1 байт)				
				- Тип события (2 байта) - только если есть событие
				- Длина данных события (2 байта) - только если есть событие
				- [НАГРУЗКА СОБЫТИЯ] Данные события  - только если есть событие
*/	

	Message m(controllerID,moduleID,Messages::EventResponse);


	// конструируем сырое сообщение	
	
	uint16_t dataLen = MESSAGE_HEADER_SIZE + 1;
	if(hasEvent)
	{
		dataLen += 4 + e->getDataLength();
	}
	
	m.payloadLength = dataLen;
	m.payload = new uint8_t[m.payloadLength];
	uint8_t* writePtr = Message::writeHeader(m.payload, controllerID, moduleID, static_cast<uint16_t>(m.type));
	
	// копируем нагрузку
	memcpy(writePtr,&hasEvent,sizeof(uint8_t));
	writePtr += sizeof(uint8_t);
	
	if(hasEvent)
	{
		// есть событие, надо скопировать его данные
		uint16_t helper16 = static_cast<uint16_t>(e->getType());
		memcpy(writePtr,&helper16,sizeof(uint16_t));
		writePtr += sizeof(uint16_t);
		
		helper16 = e->getDataLength();
		memcpy(writePtr,&helper16,sizeof(uint16_t));
		writePtr += sizeof(uint16_t);
		
		memcpy(writePtr,e->getData(),helper16);
	}
	
	return m;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Event
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Event::Event(Events _type)
{
	type = _type;
	data = NULL;
	dataLength = 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Event::Event(const Event& rhs)
{
	type = rhs.type;
	data = new uint8_t[rhs.dataLength];
	memcpy(data,rhs.data,rhs.dataLength);
	dataLength = rhs.dataLength;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Event& Event::operator=(const Event& rhs)
{
	delete [] data;
	
	type = rhs.type;
	data = new uint8_t[rhs.dataLength];
	memcpy(data,rhs.data,rhs.dataLength);
	dataLength = rhs.dataLength;
	
	return *this;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Event::~Event()
{
	delete [] data;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Event* Event::SlotDataChanged(uint8_t moduleID, AnyData* data)
{
/*
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	событие "Изменились данные исходящего слота" (SlotDataChanged)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается модулем как нагрузка сообщения EventResponse (в ответ на сообщение контроллера EventRequest) в том случае, когда данные публикуемого в системе слота - изменились на модуле, 

			при этом [НАГРУЗКА СОБЫТИЯ]:
			
				- ID модуля, инициировавшего событие
				- ID слота (2 байта, уникальный в рамках системы)
				
*/	
		
	Event* e = new Event(Events::SlotDataChanged);
	
	e->dataLength = 3;
	e->data = new uint8_t[e->dataLength];
	
	uint8_t* ptr = e->data;
	
	*ptr++ = moduleID;
		
	uint16_t t = data->getID();
	
	uint8_t*  readPtr = (uint8_t*)&t;
	for(size_t i=0;i<sizeof(uint16_t);i++)
	{
		*ptr++ = *readPtr++;
	}
	
	return e;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Event::operator==(const Event& rhs)
{
	if(&rhs == this)
		return true;
	
	if(type != rhs.type)
		return false;

	if(dataLength != rhs.dataLength || memcmp(data,rhs.data,dataLength))
		return false;
	
	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
