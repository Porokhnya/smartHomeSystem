#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <inttypes.h>
#include <Arduino.h>
#include "../core.h"
#include "../utils/vector.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
	структура пакета сообщения
	
	поскольку у нас существуют разные транспорты, мы должны корректно понимать, от какого модуля в рамках какой системы ходят данные.
	поэтому идентификация в рамках системы происходит посредством ID контроллера, а каждый модуль системы имеет свой уникальный внутренний
	номер. При регистрации модуля в контроллере модулю прописывается ID контроллера.
	
	каждое сообщение должно содержать ID контроллера, ID модуля, тип сообщения. Изменяемые данные - варьируются в зависимости от типа сообщения.
	события - являются частным случаем сообщений, и обрабатываются перед обработкой сообщений.
	
	заголовок сообщения с размерностью:
	
		ID контроллера - 4 байта
		ID модуля - 1 байт (максимум 254 модуля в системе, адрес 0xFF - широковещательный)
		Тип сообщения - 2 байта
		
	после заголовка сообщения идут:
	
		[...изменяемые данные сообщения...]
		
	
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	типы сообщений с примерной структурой пакетов (байты перечислены по порядку, начиная с нулевого).
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
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
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "я на связи" (ScanResponse)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

	отсылается дочерним модулем в ответ на сообщение "сканирую эфир" (Scan), структура:
	
		ID контроллера
		ID модуля
		Тип сообщения - "я на связи" (ScanResponse)
		нагрузка:
			- длина имени модуля (1 байт)
			- символьное имя модуля
			- кол-во исходящих слотов виртуальных данных, которые публикует модуль (1 байт)
			- кол-во входящих слотов виртуальных данных, которые слушает модуль (1 байт)

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "пинг" (Ping)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается контроллером конкретному модулю для проверки связи (на тот редкий случай, когда модуль не зарегистрировал ни одного исходящего слота), структура:
		
		ID контроллера
		ID модуля
		Тип сообщения - "пинг" (Ping)

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "понг" (Pong)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		отсылается модулем на контроллер как ответ на сообщение "пинг" (Ping), структура:

		ID контроллера
		ID модуля
		Тип сообщения - "понг" (Pong)
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "регистрация исходящего слота" (BroadcastSlotRegister)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером в эфир как запрос на регистрацию исходящего слота модуля в контроллере, структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "регистрация исходящего слота" (BroadcastSlotRegister)
			нагрузка:
				- номер исходящего слота (1 байт)
	
	
		в ответ на это сообщение модуль отсылает в эфир сообщение "данные исходящего слота" (BroadcastSlotData).

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "регистрация входящего слота" (ObserveSlotRegister)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером в эфир как запрос на регистрацию входящего слота модуля в контроллере, структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "регистрация входящего слота" (ObserveSlotRegister)
			нагрузка:
				- номер входящего слота (1 байт)
	
	
		в ответ на это сообщение модуль отсылает в эфир сообщение "данные входящего слота" (ObserveSlotData).
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные исходящего слота" (BroadcastSlotData)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем в ответ на запрос "регистрация исходящего слота" (BroadcastSlotRegister), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "данные исходящего слота" (BroadcastSlotData)
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				- тип данных слота (температура и т.п., 2 байта)
				- флаги (наличие данных и пр., 1 байт)
				- длина данных слота (2 байта)
				- данные слота

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные входящего слота" (ObserveSlotData)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем как ответ на запрос "регистрация входящего слота" (ObserveSlotRegister), структура:
		
		ID контроллера
		ID модуля
		Тип сообщения - "данные входящего слота" (ObserveSlotData)
		нагрузка:
			- ID слота (уникальный в рамках системы ID слота, 2 байта)
			- период публикации контроллером данных слота в эфир, миллисекунд (4 байта)
			
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные слота" (AnyDataBroadcast)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером в эфир для конкретного модуля, который зарегистрировал в контроллере свой входящий слот. Структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "данные слота" (AnyDataBroadcast)
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				- тип данных слота (температура и т.п., 2 байта)
				- флаги (наличие данных и пр., 1 байт)
				- длина данных слота (2 байта)
				- данные слота

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "запрос данных слота" (AnyDataRequest)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером в эфир для конкретного модуля, для запроса с него данных зарегистрированного исходящего слота. Структура:	

			ID контроллера
			ID модуля
			Тип сообщения - "запрос данных слота" (AnyDataRequest)
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "ответ данных слота" (AnyDataResponse)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается модулем как ответ на сообщение "запрос данных слота" (AnyDataRequest), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "ответ данных слота" (AnyDataResponse)
			нагрузка:
				- ID слота (уникальный в рамках системы ID слота, 2 байта)
				- тип данных слота (температура и т.п., 2 байта)
				- флаги (наличие данных и пр., 1 байт)
				- длина данных слота (2 байта)
				- данные слота

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "запрос события" (EventRequest)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается контроллером в эфир для конкретного модуля, с целью получения событий, которые хочет сообщить модуль, структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "запрос события" (EventRequest)
				
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

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение типа "Событие" (Event)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		формируется и отсылается контроллером при необходимости уведомления модулей о том или ином событии в системе. Структура:
		
			ID контроллера
			ID модуля (в общем случае 0xFF, т.е. широковещательная посылка, но может быть для конкретного модуля)
			Тип сообщения - "событие" (Event)
			нагрузка:
				- ID модуля, инициировавшего событие
				- Тип события (2 байта)
				- Длина данных события (2 байта)
				- Данные события
				
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "запрос конфигурации" (ConfigurationRequest)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером на модуль, для запроса информации о его конфигурации, структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "запрос конфигурации" (ConfigurationRequest)

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные конфигурации" (ConfigurationResponse)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
			
		отсылается модулем в ответ на событие "запрос конфигурации" (ConfigurationRequest), структура:

			ID контроллера
			ID модуля
			Тип сообщения - "данные конфигурации" (ConfigurationResponse)
			нагрузка:			
				- Кол-во слотов конфигурации (1 байт)
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "запрос слота конфигурации" (ConfigurationSlotRequest)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается контроллером на модуль для запроса определённого слота конфигурации, структура:

			ID контроллера
			ID модуля
			Тип сообщения - "запрос слота конфигурации" (ConfigurationSlotRequest)
			нагрузка:			
				- Номер слота конфигурации (1 байт)
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "данные слота конфигурации" (ConfigurationSlotResponse)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		отсылается модулем в ответ на сообщение "запрос слота конфигурации" (ConfigurationSlotRequest), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "данные слота конфигурации" (ConfigurationSlotResponse)
			нагрузка:			
				- Номер слота конфигурации (1 байт)
				- Тип слота (строковые данные, список и т.п.) (2 байта)
				- Длина имени слота (1 байт)
				- Имя слота
				- Длина данных слота (2 байта)
				- Данные слота конфигурации

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "сохранить слот конфигурации" (SaveConfigurationSlot)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером на модуль для сохранения слота конфигурации, структура:

			ID контроллера
			ID модуля
			Тип сообщения - "сохранить слот конфигурации" (SaveConfigurationSlot)
			нагрузка:			
				- Номер слота конфигурации (1 байт)
				- Длина данных слота (2 байта)
				- Данные слота конфигурации

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "слот конфигурации сохранён" (ConfigurationSlotSaved)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
				
		отсылается модулем в ответ на сообщение "сохранить слот конфигурации" (SaveConfigurationSlot), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "слот конфигурации сохранён" (ConfigurationSlotSaved)
			нагрузка:			
				- Номер слота конфигурации (1 байт)
				- Флаг успешности сохранения (1 байт)
				- Длина сообщения (1 байт)
				- Сообщение
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "запрос регистрации" (RegistrationRequest)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается контроллером в эфир для поиска модуля, находящегося в режиме регистрации, структура:
		
			ID контроллера
			ID модуля = 0xFF
			Тип сообщения - "запрос регистрации" (RegistrationRequest)
			
		если в эфире есть модуль, находящийся в режиме регистрации, он должен сохранить у себя ID контроллера, и ответить сообщением "регистрация завершена" (RegistrationResult).

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "регистрация завершена" (RegistrationResult)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается модулем в ответ на сообщение "запрос регистрации" (RegistrationRequest), структура:
		
			ID контроллера
			ID модуля
			Тип сообщения - "регистрация завершена" (RegistrationResult)

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	сообщение "список онлайн-модулей" (OnlineModulesList)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается контроллером в ответ на событие "запрос модулей онлайн" (OnlineModulesNeeded), структура:

			ID контроллера
			ID модуля
			Тип сообщения - "список онлайн-модулей" (OnlineModulesList)
			нагрузка:			
				- Битовая маска онлайн-модулей (32 байта)
		
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	СОБЫТИЯ
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		Помимо обмена данными слотов, в системе могут происходить события различного типа. Событие - частный случай сообщения, с тем отличием, что события
		обрабатываются перед тем, как перейти к обработке пула сообщений. Каждое событие рассылается контроллером на широковещательный
		адрес, однако при этом в сообщении о событии содержится информация о модуле, который его инициировал. Ниже будет приведён пример возможных событий.

		Обмен событиями происходит через контроллер, каждый модуль, желающий опубликовать в систему событие - ставит это событие во внутреннюю очередь, и отвечает 
		по запросу контроллера (сообщение EventRequest от контроллера, модуль в этом случае отвечает сообщением EventResponse).

		Контроллер, получив данные о событии посредством пары  EventRequest -> EventResponse, определяет, что делать с полученным событием. Например: если полученное событие
		типа "Изменились данные исходящего слота" - то контроллер при первой возможности запросит у модуля данные публикуемого слота. Если же полученное событие требует 
		широковещательной передачи (например, событие "Установлена связь с GSM", т.к. это событие отсылается конкретным модулем на контроллер), то контроллер при первой 
		возможности производит широковещательную передачу этого события.
		
		Т.е. модуль может опубликовать любое событие ТОЛЬКО КАК нагрузку к сообщению EventResponse ("публикация события"). В этом случае необходимо помнить, что сообщения
		типа EventResponse и Event имеют разную структуру: одно (EventResponse) предназначено как ответ контроллеру на запрос EventRequest, второе (Event) - содержит в себе
		непосредственно данные события (в то время как сообщение EventResponse может вообще не содержать данных события, когда на опрашиваемом модуле событий нет).
		
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	событие "запрос модулей онлайн" (OnlineModulesNeeded)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается модулем как нагрузка сообщения EventResponse (в ответ на сообщение контроллера EventRequest) при необходимости получения списка модулей, которые онлайн.

			при этом [НАГРУЗКА СОБЫТИЯ]:
			
				- ID модуля, инициировавшего событие
		
	
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	событие "Установлена связь с GSM" (GSMLinkAcquired)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		отсылается контроллером на широковещательный адрес, структура:
		
			ID контроллера
			ID модуля = 0xFF
			Тип сообщения - "событие" (Event)
			нагрузка:
				- Тип события - "Установлена связь с GSM" (GSMLinkAcquired)
				- ID модуля, инициировавшего событие
				- Длина данных события (2 байта)
				- Данные события
				
		если модуль хочет опубликовать это событие, то он должен отослать его в ответ на запрос EventRequest как нагрузку сообщения EventResponse, 

			при этом [НАГРУЗКА СОБЫТИЯ]:
		
				- ID модуля, инициировавшего событие
				- Длина данных события (2 байта)
				- Данные события
			
				
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	событие "Пропала связь с модулем" (ModuleLinkBroken)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
				
		отсылается контроллером на широковещательный адрес, структура:
		
			ID контроллера
			ID модуля = 0xFF
			Тип сообщения - "событие" (Event)
			нагрузка:
				- Тип события - "Пропала связь с модулем" (ModuleLinkBroken)
				- ID модуля, инициировавшего событие
				- ID модуля, с которым пропала связь
				
		если модуль хочет опубликовать это событие, то он должен отослать его в ответ на запрос EventRequest как нагрузку сообщения EventResponse, 

			при этом [НАГРУЗКА СОБЫТИЯ]:
		
				- ID модуля, инициировавшего событие
				- ID модуля, с которым пропала связь
				

	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	событие "Изменились данные исходящего слота" (SlotDataChanged)
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------

		отсылается модулем как нагрузка сообщения EventResponse (в ответ на сообщение контроллера EventRequest) в том случае, когда данные публикуемого в системе слота - изменились на модуле, 

			при этом [НАГРУЗКА СОБЫТИЯ]:
			
				- ID модуля, инициировавшего событие
				- ID слота (2 байта, уникальный в рамках системы)
		
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	другие события
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
		список событий будет расширяться, выше приведена общая концепция событий, для понимания архитектуры.
				
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	РЕГИСТРАЦИЯ МОДУЛЕЙ В СИСТЕМЕ
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		поскольку при первичном сканировании в систему попадают только зарегистрированные модули, то регистрация в системе происходит по следующему алгоритму:
		
			- модуль переводится в режим регистрации кликом на кнопку, сигнализация моргающим светодиодом
			- контроллер переводится в режим регистрации кликом на кнопку, сигнализация моргающим светодиодом
			- контроллер сканирует эфир, рассылая сообщение "запрос регистрации", и как только получит ответ от модуля, находящегося в режиме регистрации, начинает с этим модулем общение
			- если регистрация не проходит какое-то разумное время - и контроллер, и модуль должны сигнализировать об этом, например, путём выключения светодиода
			- успешная регистрация сигнализируется путём включения светодиода, светодиод можно выключить кликом на кнопку регистрации
				
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	Замечания по логике обмена
	---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	Общение контроллера с модулями должно происходить по тем транспортам, по которым конкретный модуль был обнаружен в системе !!!
	Контроллер ВСЕГДА инициирует обмен, модули ВСЕГДА выступают слейвами!!!
	Модуль должен уметь принимать решение, что со входящего слота долго нет данных !!!
	Сообщения должны иметь разумную длину !!!
	Каждый транспорт сам разбирается, как передавать сообщение - целиком или бить на части - сборка пакета на совести транспорта !!!
	Если модуль не отвечает на N запросов - он выключается из работы !!!
	Каждый транспорт САМ обеспечивает проверку целостности данных !!!
	Общение в рамках системы сводится к обслуживанию пула сообщений и событий, конкретную логику работы - реализует каждый модуль самостоятельно !!!
		
		
*/
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern "C" {
  void registration(bool result); // событие "регистрация завершена"
  void scanning(bool begin); // событие "сканирование"
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define MESSAGE_HEADER_SIZE (4+1+2) // размер заголовка любого сообщения (ID контроллера + ID модуля + тип сообщения)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class Messages : uint16_t // сообщения
{
		Unknown, // неизвестное сообщение
		Event, // сообщение типа "событие"
		Scan, // сообщение "сканирую эфир"
		ScanResponse, // сообщение "я на связи"
		Ping, // сообщение "пинг"
		Pong, // сообщение "понг"
		BroadcastSlotRegister, // сообщение "регистрация исходящего слота"
		ObserveSlotRegister, // сообщение "регистрация входящего слота"
		BroadcastSlotData, // сообщение "данные исходящего слота"
		ObserveSlotData, // сообщение "данные входящего слота"
		AnyDataBroadcast, // сообщение "данные слота"
		AnyDataRequest, // сообщение "запрос данных слота"
		AnyDataResponse, // сообщение "ответ данных слота"
		EventRequest, // сообщение "запрос события"
		EventResponse, // сообщение "публикация события"
		ConfigurationRequest, // сообщение "запрос конфигурации"
		ConfigurationResponse, // сообщение "данные конфигурации"
		ConfigurationSlotRequest, // сообщение "запрос слота конфигурации"
		ConfigurationSlotResponse, // сообщение "данные слота конфигурации"
		SaveConfigurationSlot, // сообщение "сохранить слот конфигурации"
		ConfigurationSlotSaved, // сообщение "слот конфигурации сохранён"
		RegistrationRequest, // сообщение "запрос регистрации"
		RegistrationResult, // сообщение "регистрация завершена"
		OnlineModulesList, // сообщение "список онлайн-модулей"
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class Events : uint16_t // события
{
	SlotDataChanged, // событие "Изменились данные исходящего слота"
//	OnlineModulesNeeded, // событие "запрос модулей онлайн"
//	GSMLinkAcquired, // событие "Установлена связь с GSM"
//	ModuleLinkBroken, // событие "Пропала связь с модулем"
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class AnyData; // forward declaration
class Event; // forward declaration
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Message
{
	public:
	
		uint16_t controllerID; // ID контроллера, который послал сообщение (или которому оно предназначается)
		uint8_t moduleID; // ID модуля в рамках системы
		Messages type; // тип сообщения
		
		// это событие?
		bool isEvent() const { return type == Messages::Event; }
		
		// это броадкастовое сообщение?
		bool isBroadcast() const { return moduleID == 0xFF; }
		
		// парсим из сырых данных
		static Message parse(const uint8_t* rawData, uint16_t rawDataLength);
		
		// возвращает полное сообщение в сыром виде
		const uint8_t* getPayload() const { return payload; }
		
		// возвращает длину сообщения в сыром виде
		const uint16_t getPayloadLength() const { return payloadLength; }
		
		
		// читает что-то из полезной нагрузки сообщения, начиная с указанного адреса.
		// данные головы сообщения - этим методом не читаются, и доступны в переменных
		// controllerID, moduleID, type
		

		
		template<typename T>
		T get(uint16_t payloadAddress) const
		{
			T result = T();
			
			payloadAddress += MESSAGE_HEADER_SIZE; // пропускаем заголовок, переходим к полезной нагрузке
			
			if(payloadAddress >= payloadLength)
				return result;
						
			uint8_t* writePtr = (uint8_t*)&result;
			for(size_t i=0;i<sizeof(T);i++)
			{
				*writePtr++ = payload[payloadAddress++];
			}
			
			return result;
		}
		
		uint8_t* get(uint16_t payloadAddress) const
		{
			return (payload + payloadAddress + MESSAGE_HEADER_SIZE);
		}
				
		// методы создания пакетов для различных типов сообщений
		static Message Scan(uint32_t controllerID, uint8_t moduleID);
		static Message ScanResponse(uint32_t controllerID, uint8_t moduleID, const char* moduleName, uint8_t broadcastDataCount,uint8_t observeDataCount);		
		static Message Pong(uint32_t controllerID, uint8_t moduleID);
		static Message BroadcastSlotData(uint32_t controllerID, uint8_t moduleID, AnyData* data);
		static Message ObserveSlotData(uint32_t controllerID, uint8_t moduleID, uint16_t slotID, uint32_t frequency);
		static Message AnyDataResponse(uint32_t controllerID, uint8_t moduleID, AnyData* data);
		static Message EventResponse(uint32_t controllerID, uint8_t moduleID, uint8_t hasEvent, Event* e);
		static Message RegistrationResult(uint32_t controllerID, uint8_t moduleID);
		
		// конструкторы
		Message();
		Message(uint32_t controllerID, uint8_t moduleID, Messages type);
		Message(const Message& rhs);
		Message& operator=(const Message& rhs);
		
		// деструктор
		~Message();
	
		
		
	private:
	
		static uint8_t* writeHeader(uint8_t* raw,uint32_t controllerID, uint8_t moduleID,uint16_t type);
			
		uint8_t* payload;
		uint16_t payloadLength;
		
	
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Event
{
	public:
		Event(Events type);
		Event(const Event& rhs);
		Event& operator=(const Event& rhs);
		~Event();
		
		
		Events getType() const { return type; }
		uint8_t* getData() const { return data; }
		uint16_t getDataLength() const { return dataLength; }
		
		static Event* SlotDataChanged(uint8_t moduleID, AnyData* data);
		
		
		bool operator==(const Event& rhs);
		
	private:
		Events type;
		uint8_t* data;
		uint16_t dataLength;
	
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
