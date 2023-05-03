#include "flashEEPROM.h"

static page_t pageEeprom;

/*********************************************************************
 * Получить состояние страницы                                       *
 *********************************************************************/
static uint64_t getStatePage(uint32_t address)
{
	uint64_t statePage = 0;
	for(uint8_t i = 0; i < 4; i++)
	{
		statePage = (statePage << 16) | flashRead_16(address + EEPROM_STATE_PAGE + i * 2);
	}
	return statePage;
}

/*********************************************************************
 * Записать состояние страницы                                       *
 *********************************************************************/
static uint8_t setStatePage(uint32_t address, uint64_t state)
{
	uint64_t mask = 0xFFFF000000000000;
	for(uint8_t i = 0; i < 4; i++)
	{
		if(state & mask)
		{
			if(flashRead_16(address + EEPROM_STATE_PAGE + i * 2) != 0xFFFF)
			{
				return FALSE;
			}
			continue;
		}
		if(flashRead_16(address + EEPROM_STATE_PAGE + i * 2) != 0x0000)
		{
			if(!flashWrite_16(address + EEPROM_STATE_PAGE + i * 2, 0x0000))
			{
				return FALSE;
			}
		}
		mask >>= 16;
	}
	return TRUE;
}

/*********************************************************************
 * Записать состояние данных                                         *
 *********************************************************************/
static uint8_t setStateData(uint32_t address, uint32_t state)
{
	uint32_t mask = 0xFFFF0000;
	for(uint8_t i = 0; i < 2; i++)
	{
		if(state & mask)
		{
			if(flashRead_16(address + i) != 0xFFFF)
			{
				return FALSE;
			}
			continue;
		}
		if(flashRead_16(address + i) != 0x0000)
		{
			if(!flashWrite_16(address + i, 0x0000))
			{
				return FALSE;
			}
		}
		mask >>= 16;
	}
	return TRUE;
}

/*********************************************************************
 * Поиск следующей записи в EEPROM                                   *
 *********************************************************************/
uint32_t searchNextData(uint32_t startAddress)
{
	structData_t tempData;
	flashReadDataWord(startAddress, (uint32_t*)&tempData, sizeof(tempData) / sizeof(uint32_t));
	if(tempData.stateData == NO_DATA)
	{
		return startAddress;
	}
	else
	{
		startAddress = startAddress + (3 + tempData.dataLenght) * 4;
		if(startAddress >= pageEeprom.activePage + EEPROM_PAGE_SIZE)
		{
			startAddress = 0xFFFFFFFF; // Возращается если достигли конца страницы
		}
	}
	return startAddress;
}

/*********************************************************************
 * Копирование данных с одной страницы в другую                      *
 *********************************************************************/
uint8_t copyDataPageToPage(uint32_t addressPage_1, uint32_t addressPage_2)
{
	uint32_t countAdrPage_1 = addressPage_1;
	uint32_t countAdrPage_2 = addressPage_2;
	structData_t tempData_1;
	structData_t tempData_2;
	uint32_t tempBuffer[100];
	// Поиск и удаление данных с одинаковым ID
	do{
		flashReadDataWord(countAdrPage_2, (uint32_t*)&tempData_2, sizeof(tempData_2) / sizeof(uint32_t));
		if(tempData_2.stateData == VALID_DATA)
		{
			do{
				flashReadDataWord(countAdrPage_1, (uint32_t*)&tempData_1, sizeof(tempData_1) / sizeof(uint32_t));
				if(tempData_1.stateData == VALID_DATA)
				{
					if(tempData_2.dataId == tempData_1.dataId)
					{
						setStateData(countAdrPage_2, DELETED_DATA);
						countAdrPage_2 = addressPage_2;
						break;
					}
				}
				if(tempData_1.stateData == NO_DATA)
				{
					break;
				}
				countAdrPage_1 = searchNextData(countAdrPage_1);
			}while(countAdrPage_1 != 0xFFFFFFFF);
			countAdrPage_1 = addressPage_1;
		}
		if(tempData_2.stateData == NO_DATA)
		{
			break;
		}
		countAdrPage_2 = searchNextData(countAdrPage_2);
	}while(countAdrPage_2 != 0xFFFFFFFF);
	countAdrPage_1 = addressPage_1;
	countAdrPage_2 = addressPage_2;
	do{
		flashReadDataWord(countAdrPage_1, (uint32_t*)&tempData_1, sizeof(tempData_1) / sizeof(uint32_t));
		if(tempData_1.stateData == NO_DATA)
		{
			break;
		}
		countAdrPage_1 = searchNextData(countAdrPage_1);
	}while(countAdrPage_1 != 0xFFFFFFFF);
	if(countAdrPage_1 == 0xFFFFFFFF)
	{
		return FALSE;
	}
	//Копирование данных
	do{
		flashReadDataWord(countAdrPage_2, (uint32_t*)&tempData_2, sizeof(tempData_2) / sizeof(uint32_t));
		if(tempData_2.stateData == VALID_DATA)
		{
			flashReadDataWord(addressPage_2, tempBuffer, sizeof(tempData_2) / sizeof(uint32_t) + tempData_2.dataLenght);
			if(countAdrPage_1 + sizeof(tempData_2) / sizeof(uint32_t) + tempData_2.dataLenght >= pageEeprom.activePage + EEPROM_PAGE_SIZE)
			{
				return FALSE;
			}
			if(!flashWriteDataWord(countAdrPage_1, tempBuffer, sizeof(tempData_2) / sizeof(uint32_t) + tempData_2.dataLenght))
			{
				return FALSE;
			}
			setStateData(countAdrPage_2, DELETED_DATA);
			countAdrPage_1 = countAdrPage_1 + sizeof(tempData_2) / sizeof(uint32_t) + tempData_2.dataLenght;
		}
		if(tempData_2.stateData == NO_DATA)
		{
			break;
		}
		countAdrPage_2 = searchNextData(countAdrPage_2);
	}while(countAdrPage_2 != 0xFFFFFFFF);
	return TRUE;
}

/*********************************************************************
 * Инициализация памяти                                              *
 *********************************************************************/
uint8_t flashEepromInit(void)
{
	stateMemoryPage_t stateMemoryPage_1 = getStatePage(EEPROM_PAGE_1_ADDRESS);
	stateMemoryPage_t stateMemoryPage_2 = getStatePage(EEPROM_PAGE_2_ADDRESS);
	if(stateMemoryPage_1 == PAGE_ERESED && stateMemoryPage_2 == PAGE_ERESED)
	{
		pageEeprom.activePage = EEPROM_PAGE_1_ADDRESS;
		setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_WRITE);
	}
	else if(stateMemoryPage_1 == PAGE_WRITE)
	{
		pageEeprom.activePage = EEPROM_PAGE_1_ADDRESS;
	}
	else if(stateMemoryPage_2 == PAGE_WRITE)
	{
		pageEeprom.activePage = EEPROM_PAGE_2_ADDRESS;
	}
	else if(stateMemoryPage_1 == PAGE_COPY)
	{
		pageEeprom.activePage = EEPROM_PAGE_1_ADDRESS;
		if(!copyDataPageToPage(EEPROM_PAGE_1_ADDRESS, EEPROM_PAGE_2_ADDRESS))
		{
			return FALSE;
		}
		setStatePage(EEPROM_PAGE_2_ADDRESS, PAGE_INVALID);
		setStatePage(pageEeprom.activePage, PAGE_WRITE);
		flashErasePage(EEPROM_PAGE_2_ADDRESS);

	}
	else if(stateMemoryPage_2 == PAGE_COPY)
	{
		pageEeprom.activePage = EEPROM_PAGE_2_ADDRESS;
		if(!copyDataPageToPage(EEPROM_PAGE_2_ADDRESS, EEPROM_PAGE_1_ADDRESS))
		{
			return FALSE;
		}
		setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_INVALID);
		setStatePage(pageEeprom.activePage, PAGE_WRITE);
		flashErasePage(EEPROM_PAGE_1_ADDRESS);

	}
	else
	{
		flashErasePage(EEPROM_PAGE_1_ADDRESS);
		flashErasePage(EEPROM_PAGE_2_ADDRESS);
		setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_WRITE);
	}
	return TRUE;
}

/*********************************************************************
 * Записать данные в EEPROM                                          *
 *********************************************************************/
uint8_t flashEepromWriteData(uint32_t id, uint32_t *data, uint32_t lenght)
{
	structData_t tempData;
	uint32_t tempAddr = pageEeprom.activePage;
	while(1)
	{
		flashReadDataWord(tempAddr, (uint32_t *)&tempData, (sizeof(tempData) / sizeof(uint32_t)));
		if(tempData.stateData == VALID_DATA)
		{
			if(tempData.dataId == id)
			{
				 setStateData(tempAddr, DELETED_DATA);
			}
		}
		if(tempData.stateData == NO_DATA)
		{
			if(tempAddr + sizeof(tempData) + lenght  >= pageEeprom.activePage + EEPROM_PAGE_SIZE)
			{
				setStatePage(pageEeprom.activePage, PAGE_VALID);
				if(pageEeprom.activePage == EEPROM_PAGE_1_ADDRESS)
				{
					if(getStatePage(EEPROM_PAGE_2_ADDRESS) != PAGE_ERESED)
					{
						flashErasePage(EEPROM_PAGE_2_ADDRESS);
					}
					setStatePage(EEPROM_PAGE_2_ADDRESS, PAGE_COPY);
					pageEeprom.activePage = EEPROM_PAGE_2_ADDRESS;
					tempAddr = pageEeprom.activePage;
					tempData.stateData = VALID_DATA;
					tempData.dataId = id;
					tempData.dataLenght = lenght / sizeof(uint32_t);
					if(!flashWriteDataWord(tempAddr, (uint32_t *)&tempData, sizeof(tempData) / sizeof(uint32_t)))
					{
						return FALSE;
					}
					tempAddr += sizeof(tempData);
					if(!flashWriteDataWord(tempAddr, data, tempData.dataLenght))
					{
						return FALSE;
					}
					copyDataPageToPage(EEPROM_PAGE_2_ADDRESS, EEPROM_PAGE_1_ADDRESS);
					setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_INVALID);
					setStatePage(pageEeprom.activePage, PAGE_WRITE);
					flashErasePage(EEPROM_PAGE_1_ADDRESS);
					break;
				}
				else
				{
					if(getStatePage(EEPROM_PAGE_1_ADDRESS) != PAGE_ERESED)
					{
						flashErasePage(EEPROM_PAGE_1_ADDRESS);
					}
					setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_COPY);
					pageEeprom.activePage = EEPROM_PAGE_1_ADDRESS;
					tempAddr = pageEeprom.activePage;
					tempData.stateData = VALID_DATA;
					tempData.dataId = id;
					tempData.dataLenght = lenght / sizeof(uint32_t);
					if(!flashWriteDataWord(tempAddr, (uint32_t *)&tempData, sizeof(tempData) / sizeof(uint32_t)))
					{
						return FALSE;
					}
					tempAddr += sizeof(tempData);
					if(!flashWriteDataWord(tempAddr, data, tempData.dataLenght))
					{
						return FALSE;
					}
					copyDataPageToPage(EEPROM_PAGE_1_ADDRESS, EEPROM_PAGE_2_ADDRESS);
					setStatePage(EEPROM_PAGE_2_ADDRESS, PAGE_INVALID);
					flashWriteWord(EEPROM_PAGE_1_ADDRESS, PAGE_WRITE);
					flashErasePage(EEPROM_PAGE_2_ADDRESS);
					break;
				}
			}
			else
			{
				tempData.stateData = VALID_DATA;
				tempData.dataId = id;
				tempData.dataLenght = lenght / sizeof(uint32_t);
				if(!flashWriteDataWord(tempAddr, (uint32_t *)&tempData, sizeof(tempData) / sizeof(uint32_t)))
				{
					return FALSE;
				}
				tempAddr += sizeof(tempData);
				if(!flashWriteDataWord(tempAddr, data, tempData.dataLenght))
				{
					return FALSE;
				}
				break;
			}
		}
		else
		{
			tempAddr = searchNextData(tempAddr);
			if(tempAddr == 0xFFFFFFFF || tempAddr >= pageEeprom.activePage + EEPROM_PAGE_SIZE)
			{
				setStatePage(pageEeprom.activePage, PAGE_VALID);
				if(pageEeprom.activePage == EEPROM_PAGE_1_ADDRESS)
				{
					if(getStatePage(EEPROM_PAGE_2_ADDRESS) != PAGE_ERESED)
					{
						flashErasePage(EEPROM_PAGE_2_ADDRESS);
					}
					setStatePage(EEPROM_PAGE_2_ADDRESS, PAGE_COPY);
					pageEeprom.activePage = EEPROM_PAGE_2_ADDRESS;
					tempAddr = pageEeprom.activePage;
					tempData.stateData = VALID_DATA;
					tempData.dataId = id;
					tempData.dataLenght = lenght / sizeof(uint32_t);
					if(!flashWriteDataWord(tempAddr, (uint32_t *)&tempData, sizeof(tempData) / sizeof(uint32_t)))
					{
						return FALSE;
					}
					tempAddr += sizeof(tempData);
					if(!flashWriteDataWord(tempAddr, data, tempData.dataLenght))
					{
						return FALSE;
					}
					copyDataPageToPage(EEPROM_PAGE_2_ADDRESS, EEPROM_PAGE_1_ADDRESS);
					setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_INVALID);
					setStatePage(pageEeprom.activePage, PAGE_WRITE);
					flashErasePage(EEPROM_PAGE_1_ADDRESS);
					break;
				}
				else
				{
					if(getStatePage(EEPROM_PAGE_1_ADDRESS) != PAGE_ERESED)
					{
						flashErasePage(EEPROM_PAGE_1_ADDRESS);
					}
					setStatePage(EEPROM_PAGE_1_ADDRESS, PAGE_VALID);
					pageEeprom.activePage = EEPROM_PAGE_1_ADDRESS;
					tempData.stateData = VALID_DATA;
					tempData.dataId = id;
					tempData.dataLenght = lenght / sizeof(uint32_t);
					if(!flashWriteDataWord(tempAddr, (uint32_t *)&tempData, sizeof(tempData) / sizeof(uint32_t)))
					{
						return FALSE;
					}
					tempAddr += sizeof(tempData);
					if(!flashWriteDataWord(tempAddr, data, tempData.dataLenght))
					{
						return FALSE;
					}
					copyDataPageToPage(EEPROM_PAGE_1_ADDRESS, EEPROM_PAGE_2_ADDRESS);
					setStatePage(EEPROM_PAGE_2_ADDRESS, PAGE_INVALID);
					flashWriteWord(EEPROM_PAGE_1_ADDRESS, PAGE_WRITE);
					flashErasePage(EEPROM_PAGE_2_ADDRESS);
					break;
				}
			}
		}
	}
	return TRUE;
}

/*********************************************************************
 * Прочитать данные из EEPROM                                        *
 *********************************************************************/
uint8_t flashEepromReadData(uint32_t id, uint32_t *data)
{
	structData_t tempData;
	uint32_t tempAddr = pageEeprom.activePage;
	while(1)
	{
		flashReadDataWord(tempAddr, (uint32_t *)&tempData, (sizeof(tempData) / sizeof(uint32_t)));
		if(tempData.stateData == VALID_DATA)
		{
			if(tempData.dataId == id)
			{
				flashReadDataWord(tempAddr + sizeof(tempData) , data, tempData.dataLenght);
				break;
			}
		}
		else if(tempData.stateData == NO_DATA)
		{
			return FALSE;
		}
		tempAddr = searchNextData(tempAddr);
		if(tempAddr == 0xFFFFFFFF)
		{
			return FALSE;
		}
	}
	return TRUE;
}

/*********************************************************************
 * Удалить данные из EEPROM                                          *
 *********************************************************************/
uint8_t flashEepromDeleteData(uint32_t id)
{
	structData_t tempData;
	uint32_t tempAddr = pageEeprom.activePage;
	while(1)
	{
		flashReadDataWord(tempAddr, (uint32_t *)&tempData, (sizeof(tempData) / sizeof(uint32_t)));
		if(tempData.stateData == VALID_DATA)
		{
			if(tempData.dataId == id)
			{
				 if(!setStateData(tempAddr, DELETED_DATA))
				 {
					 return FALSE;
				 }
				 break;
			}
		}
		else if(tempData.stateData == NO_DATA || tempAddr >= pageEeprom.activePage + EEPROM_PAGE_SIZE)
		{
			break;
		}
		tempAddr = searchNextData(tempAddr);
		if(tempAddr == 0xFFFFFFFF)
		{
			break;
		}
	}
	return TRUE;
}
