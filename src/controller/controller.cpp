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
}
//--------------------------------------------------------------------------------------------------------------------------------------
void SmartController::update()
{
	handleIncomingCommands();
	updateTransports();
	
	//TODO: основная логика работы контроллера !!!
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


