//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "anydata.h"
#include "../module/module.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AnyData::AnyData(DataType _type, uint16_t _id)
{
	type = type;
	id = _id;
	
	uint16_t dlen = getDataLength();
	data = new uint8_t[dlen];
	flags.triggered = false;
	flags.hasData = false;

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
AnyData::~AnyData()
{
	delete [] data;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AnyData::reset()
{
	
	// сбрасываем показания
	bool oldHasData = flags.hasData;
	flags.hasData = false;
	
	if(oldHasData  != flags.hasData)
	{
		// взводим триггер
		trigger(true);	
		// и сообщаем всем интересующимся, что у нас произошли изменения
		propagateChanges();
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool AnyData::triggered()
{
	return flags.triggered;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint16_t AnyData::getDataLength()
{
	switch(type)
	{
		case DataType::DiscreteRegister:
		case DataType::BatteryPower:
			return 1;
		
		case DataType::ADCValue:
			return 2;
			
		case DataType::Temperature:
		case DataType::SoilMoisture:
			return 3;
			
		case DataType::Luminosity:
			return 4;
			
		case DataType::Humidity:
			return 6;
	}
	
	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t AnyData::asByte()
{
	if(!(type == DataType::DiscreteRegister || type == DataType::BatteryPower) )
		return 0xFF;
	
	return data[0];
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AnyData::set(uint8_t b)
{
	if(!(type == DataType::DiscreteRegister || type == DataType::BatteryPower) )
		return;
	
	if(data[0] == b) // ничего не изменилось
		return;
	
	data[0] = b;
	trigger(true);
	propagateChanges();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Temperature AnyData::asTemperature()
{
	Temperature t;
	if(type != DataType::Temperature)
		return t;
	
	uint8_t* pData = (uint8_t*)&t.Value;
	*pData++ = data[0];
	*pData = data[1];
	
	pData = (uint8_t*)&t.Decimal;
	*pData = data[2];
	
	return t;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AnyData::set(Temperature& t)
{
	if(type != DataType::Temperature)
		return;
	
	Temperature old = asTemperature();
	
	if(old == t) // ничего не поменялось, не надо отсылать в сеть
		return;
	
	flags.hasData = true;

	uint8_t* pData = (uint8_t*) &t.Value;
	
	data[0] = *pData++;
	data[1] = *pData;
	
	pData = (uint8_t*)&t.Decimal;
	data[2] = *pData;	
	
	trigger(true);
	propagateChanges();

	
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AnyData::propagateChanges()
{
	if(_Module)
	{
		DBGLN(F("Inform module about data changes!"));
		
		// сообщаем модулю, что данные изменились
		_Module->informDataChanged(this);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AnyData::setRaw(DataType rawType, const uint8_t* rawData, uint16_t rawDataSize)
{
	if(rawType != type || !rawData || !rawDataSize)
	{
		DBGLN(F("[ERR] - bad data params!"));
		return;
	}
	
	uint16_t dlen = getDataLength();
	if(rawDataSize != dlen)
	{
		DBGLN(F("[ERR] - different sizes!"));
		return;
	}
	
	flags.hasData = true;
	
	bool hasChanges = !memcmp(data,rawData,dlen);
	
	memcpy(data,rawData,dlen);
	
	if(hasChanges)
	{
		trigger(true);
	}
	
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
