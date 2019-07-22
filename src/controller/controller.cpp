#include "controller.h"
#include "../utils/uptime.h"
//--------------------------------------------------------------------------------------------------------------------------------------
// список поддерживаемых команд
//--------------------------------------------------------------------------------------------------------------------------------------
const char ID_COMMAND[] PROGMEM = "ID"; // получить ID контроллера (GET=ID)
const char UPTIME_COMMAND[] PROGMEM = "UPTIME"; // получить время работы контроллера, секунд (GET=UPTIME)
//--------------------------------------------------------------------------------------------------------------------------------------
SmartController::SmartController(uint32_t _id, const char* _name, _Storage& _storage)
{
	controllerID = _id;
	name = _name;
	storage = &_storage;
}
//--------------------------------------------------------------------------------------------------------------------------------------
SmartController::~SmartController()
{
	for(size_t i=0;i<modulesList.size();i++)
	{
		delete modulesList[i];
	}
	//TODO: очистка памяти !!!
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::begin()
{
	DBGLN(F("[C] begin."));
	
	// вычитываем сохранённый ID контроллера
	uint32_t savedID;
	if(StorageReader::read(storage,0,savedID))
	{
		DBG(F("[C] Saved ID: "));
		DBGLN(savedID);
		controllerID = savedID;
	}

	
	// стартуем все транспорты
	for(size_t i=0;i<transports.size();i++)
	{
		transports[i]->begin();
	}
	
	//TODO: запуск остального !!!
	
	// начинаем сканировать эфир
	scan();
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::updateScan()
{
	//TODO: тут обновление сканирования!
	switch(scanState)
	{
		case ScanState::AskModule:
		{
			DBG(F("[C] Scan module #"));
			DBG(currentModuleIndex);
			DBG(F(" at transport #"));
			DBGLN(currentTransportIndex);
			
			// конструируем собщение
			Message m = Message::Scan(controllerID, currentModuleIndex);

			timeout = transports[currentTransportIndex]->getReadingTimeout();
			timer = uptime();
			scanState = ScanState::WaitForModuleAnswer;
			
			// публикуем в транспорт запрос к модулю
			transports[currentTransportIndex]->write(m.getPayload(),m.getPayloadLength());								
			
		}
		break; // ScanState::AskModule
		
		case ScanState::WaitForModuleAnswer:
		{
			// посылали запрос модулю, надо проверить, есть ли ответ и нет ли таймаута?
			if(uptime() - timer >= timeout)
			{
				// модуль не отвечает по таймауту
				DBGLN(F("[C] Module not answering!"));
				
				// переходим на следующий модуль
				currentModuleIndex++;
				scanState = ScanState::AskModule;
				
				if(currentModuleIndex == 0xFF)
				{
					// добрались до широковещательного адреса, переходим на следующий транспорт
					DBGLN(F("[C] Switch to next transport!"));
					currentTransportIndex++;
					currentModuleIndex = 0;
				}
				
				if(currentTransportIndex >= transports.size())
				{
					// сканирование закончено!
					scanDone = true;
				}
			}
			else
			{
				// проверяем, есть ли в транспорте входящий пакет?
				if(transports[currentTransportIndex]->available())
				{
					// есть входящий пакет
					uint16_t payloadLength;
					uint8_t* payload = transports[currentTransportIndex]->read(payloadLength);
					
					// тут парсим сообщение и понимаем, что к чему
					Message incoming = Message::parse(payload,payloadLength);
					
					// говорим транспорту, что мы больше не нуждаемся в пакете
					transports[currentTransportIndex]->wipe();
					
					// парсим входящий пакет
					if(incoming.type == Messages::ScanResponse && incoming.controllerID == controllerID && incoming.moduleID == currentModuleIndex)
					{
						DBG(F("[C] Catch online module with index="));
						DBGLN(currentModuleIndex);
						
						//тут помещаем модуль в список онлайн модулей
						ControllerModuleInfo* minf = new ControllerModuleInfo(currentModuleIndex,transports[currentTransportIndex]);
						modulesList.push_back(minf);
					}
					
					// переходим на следующий модуль
					currentModuleIndex++;
					scanState = ScanState::AskModule;
				
					if(currentModuleIndex == 0xFF)
					{
						// добрались до широковещательного адреса, переходим на следующий транспорт
						DBGLN(F("[C] Switch to next transport!"));
						currentTransportIndex++;
						currentModuleIndex = 0;
					}
					
					if(currentTransportIndex >= transports.size())
					{
						// сканирование закончено!
						scanDone = true;
					}
				} // available
			} // else
		}
		break; // ScanState::WaitForModuleAnswer
		
	} // switch
	
	if(scanDone)
	{
		DBGLN(F("[C] Scan done, ask for slots!"));
		askSlots();
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::scan()
{
	DBGLN(F("[C] Start scan..."));
	
	scanDone = !transports.size();
	if(scanDone)
	{
		DBGLN(F("[C] Scan done (transports == 0), asks for slots!"));
		askSlots();
		return;
	}
	
	// чистим список модулей онлайн
	for(size_t i=0;i<modulesList.size();i++)
	{
		delete modulesList[i];
	}	
	modulesList.empty();

	machineState = SmartControllerState::Scan; // переключаемся на ветку сканирования модулей
	currentTransportIndex = 0;
	currentModuleIndex = 0;
	scanState = ScanState::AskModule;
	
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::askSlots()
{
	DBGLN(F("[C] Ask for slots!"));
	
	machineState = SmartControllerState::AskSlots; // переключаемся на ветку опроса слотов у всех онлайн-модулей
	
	//TODO: тут инициализация перед началом сканирования слотов!
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::updateAskSlots()
{
	//TODO: тут обновление сканирования слотов!
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::update()
{
	handleIncomingCommands();
	updateTransports();
	
	switch(machineState)
	{
		case SmartControllerState::Scan:
		{
			// сканируем эфир, обновляем эту ветку
			updateScan();
		}
		break; // SmartControllerState::Scan
		
		case SmartControllerState::AskSlots:
		{
			updateAskSlots();
		}
		break; // SmartControllerState::AskSlots
		
		case SmartControllerState::Normal:
		{
			//TODO: Нормальный режим работы !!!
		}
		break; // SmartControllerState::Normal
		
	} // switch
	
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::updateTransports()
{
	// обновляем все транспорты
	for(size_t i=0;i<transports.size();i++)
	{
		transports[i]->update();
	}	
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::startRegistration(uint32_t timeout)
{
	DBGLN(F("[C] Start registration!"));
	//TODO: запуск процесса поиска модуля в режиме регистрации !!!
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::addTransport(Transport& t)
{
	transports.push_back(&t);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::addStreamListener(StreamListener& sl)
{
	listeners.push_back(&sl);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::handleIncomingCommands()
{
	for(size_t i=0;i<listeners.size();i++)
	{
		if(listeners[i]->hasCommand())
		{
			String command = listeners[i]->getCommand();
			
			if(command.startsWith(CORE_COMMAND_GET) || command.startsWith(CORE_COMMAND_SET))
			{
			  Stream* pStream = listeners[i]->getStream();
			  processCommand(command,pStream);
			}
			
			listeners[i]->clearCommand();
		}
		
	} // for
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::processCommand(const String& command, Stream* answerTo)
{
	if(command.startsWith(CORE_COMMAND_SET)) // SET=...
	{
			setCommand(command,answerTo);
	}
	else 
		if(command.startsWith(CORE_COMMAND_GET)) // GET=...
		{
			getCommand(command,answerTo);
		}
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::setCommand(const String& command, Stream* answerTo)
{
	bool handled = false;
	CommandParser cParser;
	
     if(cParser.parse(command,true))
     {
        const char* commandName = cParser.getArg(0);
		
		//TODO: ТУТ обработка команд на установку свойств !!!
		
	 }
	 
	 if(!handled)
		 unknownCommand(answerTo);
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::getCommand(const String& command, Stream* answerTo)
{
	bool handled = false;
	CommandParser cParser;
	
	 if(cParser.parse(command,false))
	 {
		const char* commandName = cParser.getArg(0);
		
		if(!strcmp_P(commandName, ID_COMMAND)) // GET=ID, returns OK=ID|id|name
		{
			okAnswer(answerTo, commandName) << controllerID << CORE_COMMAND_PARAM_DELIMITER << name << ENDLINE;
			handled = true;
		}
		else
			if(!strcmp_P(commandName, UPTIME_COMMAND)) // GET=UPTIME, returns OK=UPTIME|uptime
			{
				okAnswer(answerTo, commandName) << uint32_t(uptime()/1000ul) << ENDLINE;
				handled = true;
			}
			
		//TODO: ТУТ ОСТАЛЬНЫЕ КОМАНДЫ ЧТЕНИЯ СВОЙСТВ !!
	 }
	 
	 if(!handled)
		 unknownCommand(answerTo);	 
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::unknownCommand(Stream* answerTo)
{
	*answerTo << CORE_COMMAND_ANSWER_ERROR << F("UNKNOWN_COMMAND") << ENDLINE;
}
//--------------------------------------------------------------------------------------------------------------------------------------
Stream& SmartController::okAnswer(Stream* answerTo, const char* command)
{
	*answerTo << CORE_COMMAND_ANSWER_OK << command << CORE_COMMAND_PARAM_DELIMITER;
	return *answerTo;
}
//--------------------------------------------------------------------------------------------------------------------------------------


