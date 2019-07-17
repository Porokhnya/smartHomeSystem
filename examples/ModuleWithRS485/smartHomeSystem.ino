//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ВСЯКИЕ ГЛОБАЛЬНЫЕ НАСТРОЙКИ ЯДРА - В ФАЙЛЕ src/config.h !!!
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// настройки прошивки
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define RS485_SERIAL Serial1 // какой Serial использовать для RS-485 ?
#define SERIAL_SPEED 57600 // скорость работ всех задействованных Serial

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "src/core.h" // подключаем ядро
#include "src/storage/eeprom.h" // будем использовать встроенную память EEPROM
#include "src/transport/rs485.h" // будем использовать RS-485 как транспорт
#include "src/utils/button.h" // подключаем поддержку кнопки с антидребезгом
#include "src/utils/trigger.h" // подключаем поддержку триггера (смена состояния через определённые промежутки времени)
#include "src/module/module.h" // будем работать, как модуль

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// наши переменные
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// транспорт
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RS485 rs485(RS485_SERIAL, 4, 20); // работаем по RS-485 через Serial, пин переключения приема-передачи - 4, 20 миллисекунд на таймаут чтения входящих данных

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// хранилище
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Storage storage; // хранилище данных, где будет хранитьcя всякую служебную информацию (его использование обязательно!)

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// кнопка регистрации и светодиод регистрации
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
button regBtn; // кнопка, которую мы будем использовать для регистрации в системе
const uint8_t regBtnPin = 12; // номер пина для кнопки регистрации
const uint8_t ledPin = 13; // номер пина для светодиода сигнализации процесса регистрации
trigger regTrigger; // триггер, который мы будем использовать для моргания светодиода при процессе регистрации

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ПЕРЕМЕННЫЕ ДЛЯ МОДУЛЯ
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  
// модуль, обеспечивающий работу в рамках системы
SmartModule module("датчики", 1, rs485, storage); // имя модуля, уникальный ID модуля (0-254), транспорт для модуля, хранилище для модуля

// значение, которое мы будем публиковать в системе
AnyData myTemperature(DataType::Temperature,1); // тип данных (см. src/data/anydata.h, enum DataType), уникальный ID в системе (0-65535)

// значение, которое мы будем слушать из системы
AnyData remoteFlag(DataType::Byte,2); // байтовое значение, уникальный ID в системе (0-65535)
  

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void registration(bool result) // событие "результат регистрации"
{
  
  // останавливаем триггер мигающего светодиода
  regTrigger.stop();
  
  if(result)
  {
    DBGLN(F("REGISTRATION DONE!"));
    // гасим светодиод, сигнализирующий о процессе успешного завершения регистрации
    Pin::write(ledPin,LOW);
  }
  else
  {
    DBGLN(F("REGISTRATION ERROR!!!"));
    // зажигаем светодиод, сигнализируя о неуспешности регистрации
    Pin::write(ledPin,HIGH);
  }

  /*
   Поведение светодиода: 

    1. Модуль: если не зарегистрирован - горит. В процессе регистрации - мигает. Если регистрация успешна - гаснет и не горит;
    2. Контроллер: не горит. В процессе регистрации - мигает. Если регистрация неуспешна - горит.

   */
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  
  // если дебаг-режим включен - поднимает дебаг-сериал
  #ifdef _DEBUG
    DEBUG_SERIAL.begin(SERIAL_SPEED);
  #endif
  
  // поднимаем нужный Serial, поскольку мы используем RS-485 как транспорт
  RS485_SERIAL.begin(SERIAL_SPEED);

  // инициализируем хранилище
  storage.init(100); // с адреса 100 будем хранить служебные данные модуля или контроллера

  regBtn.begin(regBtnPin,true); // кнопка регистрации на пине regBtnPin, с подтяжкой к питанию, срабатывает по низкому уровню
  Pin::mode(ledPin,OUTPUT); // будем использовать светодиод на пине ledPin как сигнализатор процесса регистрации
  
  // привязываем наши данные к системе

  // сообщаем, что один вид данных мы будем публиковать
  module.broadcast(myTemperature);

  // а другой - слушать на предмет изменений
  // говорим, что контроллер будет посылать нам данные этого регистра раз в 5 секунд (только если они изменились),
  // и если в течение 30 секунд данные не приходят с контроллера - то показания сбросятся в вид "нет данных"
  module.observe(remoteFlag,5000ul,30000ul); 

  module.begin(); // модуль готов к работе, стартуем его

  // можем при старте принудительно привязать модуль к контроллеру, без дополнительной регистрации,
  // это делается ТОЛЬКО после вызова Module::begin()
  module.linkToController(1234ul); // параметр - уникальный ID контроллера


  // после всех манипуляций - при старте проверяем, зарегистрирован ли модуль в системе.
  // если да - то гасим светодиод регистрации, иначе - зажигаем.
  if(module.registered())
    Pin::write(ledPin,LOW);
  else
    Pin::write(ledPin,HIGH);


	//TODO: ВАША ЛОГИКА ЗДЕСЬ !!!
  

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop()
{

  regBtn.update(); // обновляем кнопку регистрации

  // проверяем триггер моргающего светодиода регистрации на срабатывание
  if(regTrigger.triggered())
  {
    // триггер сработал, меняем состояние светодиода на противоположное
    Pin::write(ledPin,!Pin::read(ledPin));
  }

  // если кнопка регистрации зажата и удерживается - начинаем процесс регистрации
  if(regBtn.isRetention())
  {   
    // начинаем моргать светодиодом
    regTrigger.start(500); // триггер будет срабатывать каждые 500 миллисекунд
    module.startRegistration(10000ul); // начинаем регистрацию с таймаутом в 10 секунд
  }
  
  
  module.update(); // обновляем модуль
  
  // В ЗАКОММЕНТИРОВАННОМ КУСКЕ КОДА НИЖЕ - ПОКАЗАНО, КАК РАБОТАТЬ С ОБЩИМИ ДАННЫМИ СИСТЕМЫ

  /*
  // для теста - раз в 10 секунд будем публиковать фейковую температуру
  static uint32_t past = 0;
  if(millis() - past >= 10000ul)
  {
    static int16_t cntr = 0;
    cntr++;
    if(cntr > 200) // идём от 1 до 200 градусов
      cntr = 1;

    // формируем температуру
    Temperature t{cntr,0};

    // и сообщаем системе текущие показания
    myTemperature.set(t);
      
    past = millis();
  }

  // теперь проверяем, есть ли изменения в данных удалённого датчика в системе
  if(remoteFlag.triggered())
  {
     // да, данные изменились, проверяем, какие они
     if(remoteFlag.hasData())
     {
       // есть показания, выводим их
       DBG(F("REMOTE REGISTER STATE: "));
       DBGLN(remoteFlag.asByte());
     }
     else
     {
      // нет показаний, авария!
      DBGLN(F("REMOTE REGISTER HAS NO DATA !!!"));
     }
  }
  */

  //TODO: ВАША ЛОГИКА ЗДЕСЬ !!!
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void yield()
{
  // поскольку работа ядра - асинхронная, мы должны обязательно обновлять модуль в функции yield, чтобы не потерять входящих данных
   module.update(); 
   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
