#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "../storage/storage.h"
#include "../transport/transport.h"
#include "../message/message.h"
#include "../utils/vector.h"
#include "../data/anydata.h"

#include <inttypes.h>
#include <limits.h>
#include <Arduino.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class AnyData; // forward declaration
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
	AnyData* data;
	uint32_t lastDataAt; // когда получили последние данные?
	uint32_t timeout; // таймаут до сброса слота в вид "нет данных", если с контроллера долго не приходят данные
	uint32_t frequency; // частота, с которой контроллер будет нам направлять данные этого слота
	
} AnyDataTimer;
#pragma pack(pop)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<AnyData*> AnyDataList;
typedef Vector<AnyDataTimer> AnyDataTimerList;
typedef Vector<Event*> EventsList;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class SmartModule
{
	public:
	
		SmartModule(const char* moduleName, uint8_t moduleID, Transport& t, _Storage& s);
		~SmartModule();
		
		
		// принудительно привязывает модуль к контроллеру
		void linkToController(uint32_t controllerID);

		// возвращает true, если модуль привязан к какому-либо контроллеру
		bool registered();
				
		// начинаем работу
		void begin();
		
		// обновляем состояние
		void update();
		
		// добавление публикации данных
		void broadcast(AnyData& data);
		
		// добавление наблюдения за данными
		void observe(AnyData& data, uint32_t observeFrequency, uint32_t resetTimeout=ULONG_MAX);
		
		uint8_t getID() { return moduleID; }
		uint32_t getControllerID() { return controllerID; }
		const char* getModuleName() { return moduleName; }
		
		// начинаем регистрацию
		void startRegistration(uint32_t timeout);
		
	protected:
	
		friend class AnyData;
		
		// ВНУТРЕННИЕ ФУНКЦИИ, НЕ ДЛЯ ВНЕШНЕГО ИСПОЛЬЗОВАНИЯ !!!
		void informDataChanged(AnyData* data);
		
		
	private:
	
		void processIncomingMessage();
		
		bool toMe(const Message& m);
		
		void processEvent(const Message& m);
		void processMessage(const Message& m);
		
		void updateObserveSlot(const Message& m);
			
		_Storage* storage;
		Transport* transport;
		
		const char* moduleName;
		uint8_t moduleID;
	
		uint32_t controllerID;
		bool canWork;
		
		AnyDataList broadcastList;
		AnyDataTimerList observeList;
		
		// registration related
		bool inRegMode;
		uint32_t regTimeout, regStartedAt;
		uint32_t oldControllerID;
		
		
		EventsList events;
		bool eventExists(Event* e);
		void addEvent(Events type, AnyData* data);
		Event* getEvent();
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern SmartModule* _Module; // рабочий экземпляр модуля