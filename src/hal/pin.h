#pragma once

#include <Arduino.h>

class Pin
{
	public:
	
		static void mode(uint8_t pin, uint8_t mode) { pinMode(pin,mode); }
		static void write(uint8_t pin, uint8_t level) { digitalWrite(pin,level); }
		static uint8_t read(uint8_t pin) { return digitalRead(pin); }	
	
};
