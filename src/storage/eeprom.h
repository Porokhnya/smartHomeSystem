#pragma once
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "storage.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <inttypes.h>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Storage : public _Storage
{
	public:
	
	Storage() {baseAddress = 0;}
	
	void init(uint16_t _baseAddress) { baseAddress = _baseAddress;}
	uint8_t read(uint16_t address) { return EEPROM.read(baseAddress + address); }
	void write(uint16_t address,uint8_t val) { EEPROM.write(baseAddress + address,val); }
	
	private:
		uint16_t baseAddress;
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
