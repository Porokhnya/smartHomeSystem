#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <inttypes.h>
#include "../module/module.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// типы виртуальных данных
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum class DataType : uint8_t
{
	Temperature, // температура (3 байта)
	Humidity,	// влажность+температура (6 байт)
	Luminosity,	// освещённость (4 байта)
	SoilMoisture, // влажность почвы (3 байта)
	DiscreteRegister, // состояние (0,1) 1 байт
	ADCValue, // значение АЦП (2 байта)
	BatteryPower, // заряд батареи (1 байт)
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// удобная работа с данными
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct Temperature
{
	int16_t Value;
	uint8_t Decimal;
	
	int32_t raw(const Temperature& rhs)
	{
		int32_t result = rhs.Value*100;
		if(result < 0)
			result -= rhs.Decimal;
		else
			result += rhs.Decimal;
		
		return result;
	}
	
	bool operator >(const Temperature& rhs)
	{
		if(&rhs == this)
			return false;
		
		return raw(*this) > raw(rhs);
	}

	bool operator >=(const Temperature& rhs)
	{
		if(&rhs == this)
			return false;
		
		return raw(*this) >= raw(rhs);
	}
	
	bool operator <(const Temperature& rhs)
	{
		if(&rhs == this)
			return false;
		
		return raw(*this) < raw(rhs);
	}
	
	bool operator <=(const Temperature& rhs)
	{
		if(&rhs == this)
			return false;
		
		return raw(*this) <= raw(rhs);
	}

	bool operator ==(const Temperature& rhs)
	{
		if(&rhs == this)
			return false;
		
		return raw(*this) == raw(rhs);
	}

	bool operator !=(const Temperature& rhs)
	{
		return !(operator==(rhs));
	}
	
};
#pragma pack(pop)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef Temperature SoilMoisture;
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct Humidity
{
	Temperature T; // температура
	Temperature H; // влажность
};
#pragma pack(pop)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push,1)
typedef struct
{
	bool hasData : 1;
	bool triggered : 1;
	
} AnyDataFlags;
#pragma pack(pop)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
class AnyData
{
	public:
		 AnyData(DataType type, uint16_t id);
		~AnyData();
		
		// доступ к данным температуры
		Temperature asTemperature();
		void set(Temperature& t);
		
		// доступ к данным дискретного регистра
		uint8_t asByte();
		void set(uint8_t b);
		
		// есть ли данные?
		bool hasData() { return flags.hasData; }
		
		// сбрасываем показания на вид "нет данных"
		void reset();
		
		uint16_t getID() { return id; }
		DataType getType() { return type; }
		
		uint16_t getDataLength();
		uint8_t* getData() { return data; }
		
		bool triggered();
		
		bool operator==(const AnyData& rhs)
		{
			return (this->id == rhs.id);
		}
		
	protected:
	
		friend class SmartModule;

		// СЛУЖЕБНЫЕ МЕТОДЫ !!!
		void setRaw(DataType rawType, const uint8_t* rawData, uint16_t rawDataSize);
		void trigger(bool b) { flags.triggered = b; }
		
	private:
	

		DataType type;
		uint16_t id;		
		uint8_t* data;
		AnyDataFlags flags;
		
		void propagateChanges();
		

		AnyData(const AnyData& rhs);
		AnyData& operator=(const AnyData& rhs);
		
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
