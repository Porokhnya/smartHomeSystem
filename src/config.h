#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define _DEBUG
#define DEBUG_SERIAL Serial
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define SETT_HEADER1 0x24 // байты, сигнализирующие о наличии сохранённых настроек, первый
#define SETT_HEADER2 0x19 // и второй
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _DEBUG
  #define DBG(s) { DEBUG_SERIAL.print((s)); }
  #define DBGLN(s) { DEBUG_SERIAL.println((s)); }
#else
  #define DBG(s) (void) 0
  #define DBGLN(s) (void) 0
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
