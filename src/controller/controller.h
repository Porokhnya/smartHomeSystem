#pragma once

#include <Arduino.h>
#include "../utils/vector.h"
#include "../config.h"
#include <stddef.h>
#include "streamlistener.h"
#include "../storage/storage.h"
#include "../transport/transport.h"
//--------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<StreamListener*> ListenersList;
typedef Vector<Transport*> TransportsList;
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
	
		uint32_t controllerID;
		const char* name;
		_Storage* storage;
		
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
