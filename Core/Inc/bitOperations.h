#ifndef _BIT_OPERATIONS_H_
#define BitCtr(reg, bitNamber) reg &= ~(1 << (bitNamber))			//Сбросить бит регистра reg в "0"
#define BitSet(reg, bitNamber) reg |= (1 << (bitNamber))			//Установить бит регистра reg в "1"
#define BitStatus(reg, bitNamber) ((reg) & (1 << (bitNamber)))		//Проверка бита регистра reg
#define TRUE 1														//Истина
#define FALSE (!TRUE)												//Ложь
#endif
