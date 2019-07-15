#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <inttypes.h>
#include <stddef.h>
#include "../core.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class _Storage
{
	public:
		virtual void init(uint16_t baseAddress) = 0;
		virtual uint8_t read(uint16_t address) = 0;
		virtual void write(uint16_t address, uint8_t val) = 0;
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class StorageReader
{

public:

    template<typename T >
    static bool read(_Storage* s, uint16_t address, T& result)
    {
		if(!s)
			return false;
		
        if(s->read(address++) != SETT_HEADER1)
          return false;
    
        if(s->read(address++) != SETT_HEADER2)
          return false;
    
       uint8_t* ptr = (uint8_t*)&result;
    
       for(size_t i=0;i<sizeof(T);i++)
       {
        *ptr++ = s->read(address++);
       }
    
      return true;      
    }

    template<typename T >
    static void write(_Storage* s, uint16_t address, T& val)
    {
		if(!s)
			return;
		
        s->write(address++,SETT_HEADER1);
        s->write(address++,SETT_HEADER2);
      
        uint8_t* ptr = (uint8_t*)&val;
        
        for(size_t i=0;i<sizeof(T);i++)
          s->write(address++,*ptr++);      
    }
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
