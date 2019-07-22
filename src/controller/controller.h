#pragma once

#include <Arduino.h>
#include "../utils/vector.h"
#include "../config.h"
#include <stddef.h>
#include "streamlistener.h"
#include "../storage/storage.h"
#include "../transport/transport.h"
#include "../message/message.h"
#include "../data/anydata.h"
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<StreamListener*> ListenersList;
typedef Vector<Transport*> TransportsList;
//--------------------------------------------------------------------------------------------------------------------------------------
// состояния конечного автомата работы контроллера
enum class SmartControllerState
{
	Normal, // нормальное состояние
	Scan, // сканирует эфир
	AskSlots, // опрашивает настройки слотов модулей
};
//--------------------------------------------------------------------------------------------------------------------------------------
// состояния конечного автомата сканирования
enum class ScanState
{
	AskModule,
	WaitForModuleAnswer
};
//--------------------------------------------------------------------------------------------------------------------------------------
// информация о модуле в системе
struct ControllerModuleInfo
{
		ControllerModuleInfo(uint8_t mid, Transport* t)
		{
			moduleID = mid;
			transport = t;
		}
		uint8_t moduleID; // ID модуля
		Transport* transport; // транспорт для модуля
	//TODO: тут будет другая информация, типа слотов для модуля
}; 
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<ControllerModuleInfo*> SmartModulesList;
//--------------------------------------------------------------------------------------------------------------------------------------
class SmartController
{
	public:
		SmartController(uint32_t id, const char* name, _Storage& storage);
		~SmartController();
		
		void begin();
		void update();
		
		void startRegistration(uint32_t timeout);
		
		void addTransport(Transport& t);
		void addStreamListener(StreamListener& sl);
		
		uint32_t getID() { return controllerID; }
		const char* getName() { return name; }
		
	private:
	
		SmartControllerState machineState;
		
		void scan();
		void updateScan();
		
		void askSlots();
		void updateAskSlots();
		
		ScanState scanState;
		uint8_t currentTransportIndex;
		uint8_t currentModuleIndex;
		uint32_t timer, timeout;
		bool scanDone;
	
		uint32_t controllerID;
		const char* name;
		_Storage* storage;
		
		SmartModulesList modulesList; // список модулей онлайн
		
		TransportsList transports;
		void updateTransports();
		
		ListenersList listeners;
		void handleIncomingCommands();
		void processCommand(const String& command, Stream* answerTo);
		void setCommand(const String& command, Stream* answerTo);
		void getCommand(const String& command, Stream* answerTo);
		void unknownCommand(Stream* answerTo);
		Stream& okAnswer(Stream* answerTo, const char* command);
	
};
//--------------------------------------------------------------------------------------------------------------------------------------
