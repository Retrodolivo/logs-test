#include "flashMemory.h"

/*********************************************************************
 * Разблокировать флеш для записи                                    *
 *********************************************************************/
void flashUnlock(void)
{
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;
}

/*********************************************************************
 * Заблокировать флеш для записи                                     *
 *********************************************************************/
void flashLock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}

/*********************************************************************
 * Получить состояние флеш FALSE - занята                            *
 *********************************************************************/
uint8_t flasReady(void)
{
	return !(FLASH->SR & FLASH_SR_BSY);
}

/*********************************************************************
 * Проверка окончания операции                                       *
 *********************************************************************/
uint8_t flasCheckEOP(void)
{
	if(FLASH->SR & FLASH_SR_EOP)
	{
		FLASH->SR |= FLASH_SR_EOP;
		return TRUE;
	}
	return FALSE;
}

/*********************************************************************
 * Стереть страницу флеш                                             *
 *********************************************************************/
uint8_t flashErasePage(uint32_t address)
{
	while(!flasReady());
	flashUnlock();
	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = address;
	FLASH->CR |= FLASH_CR_STRT;
	while(!flasReady());
	FLASH->CR &= ~FLASH_CR_PER;
	flashLock();
	return flasCheckEOP();
}

/*********************************************************************
 * Запись одного слова в флеш. Данные пишутся по 16 бит              *
 *********************************************************************/
uint8_t flashWriteWord(uint32_t address, uint32_t data)
{
	while(!flasReady());
	flashUnlock();
	FLASH->CR |= FLASH_CR_PG;
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!flasReady());
	if(!flasCheckEOP()) return FALSE;
	address += 2;
	*(__IO uint16_t*)address = (uint16_t)(data >> 16);
	while(!flasReady());
	FLASH->CR &= ~(FLASH_CR_PG);
	flashLock();
	return flasCheckEOP();
}

/*********************************************************************
 * Запись 16 бит в флеш.                                             *
 *********************************************************************/
uint8_t flashWrite_16(uint32_t address, uint16_t data)
{
	while(!flasReady());
	flashUnlock();
	FLASH->CR |= FLASH_CR_PG;
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!flasReady());
	FLASH->CR &= ~(FLASH_CR_PG);
	flashLock();
	return flasCheckEOP();
}

/*********************************************************************
 * Запись данных длинною lenght в флеш. Данные пишутся по 16 бит     *
 *********************************************************************/
uint8_t flashWriteDataWord(uint32_t address, uint32_t *data, uint16_t lenght)
{
	while(lenght)
	{
		if(!flashWriteWord(address, *data)) return FALSE;
		data++;
		address += 4;
		lenght--;
	}
	return TRUE;
}

/*********************************************************************
 * Чтение 8 бит данных из флеш                                       *
 *********************************************************************/
uint8_t flashRead_8(uint32_t address)
{
	return (*(__IO uint8_t*) address);
}

/*********************************************************************
 * Чтение 16 бит данных из флеш                                       *
 *********************************************************************/
uint16_t flashRead_16(uint32_t address)
{
	return (*(__IO uint16_t*) address);
}

/*********************************************************************
* Чтение данных из флеш                                             *
*********************************************************************/
uint32_t flashReadWord(uint32_t address)
{
	return (*(__IO uint32_t*) address);
}

/************************************************************************/
/* Чтение данных длинною в lenght слов  из FLASH                        */
/************************************************************************/
void flashReadDataWord(uint32_t startAddresData, uint32_t *buffer, uint32_t lenght)
{
	uint32_t temp;
	while(lenght)
	{
		temp = (*(__IO uint32_t *)startAddresData);
		*buffer = temp;
		startAddresData += 4;
		buffer++;
		lenght--;
	}
}
