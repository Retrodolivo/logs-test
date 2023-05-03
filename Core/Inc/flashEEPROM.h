#ifndef _FLASHEEPROM_H
#define _FLASHEEPROM_H

#include "main.h"
#include "flashMemory.h"
#include "bitOperations.h"

#define EEPROM_HIDER_SIZE			8
#define EEPROM_PAGE_1_ADDRESS		0x0803E800
#define EEPROM_PAGE_2_ADDRESS		EEPROM_PAGE_1_ADDRESS + FLASH_PAGE_SIZE
#define EEPROM_PAGE_SIZE			FLASH_PAGE_SIZE - EEPROM_HIDER_SIZE

#define EEPROM_STATE_PAGE			EEPROM_PAGE_SIZE

typedef enum StateMemoryPage{
	PAGE_ERESED = 0xFFFFFFFFFFFFFFFF,
	PAGE_COPY = 0x0000FFFFFFFFFFFF,
	PAGE_WRITE = 0x00000000FFFFFFFF,
	PAGE_VALID = 0x000000000000FFFF,
	PAGE_INVALID = 0x0000000000000000
}stateMemoryPage_t;

typedef enum StateData{
	NO_DATA = 0xFFFFFFFF,
	VALID_DATA = 0x0000FFFF,
	DELETED_DATA = 0x00000000
}stateData_t;

typedef struct {
	uint32_t activePage;
	uint32_t nextWriteAddress;
}page_t;


typedef struct StructData{
	stateData_t stateData;	// Статус данных
	uint32_t dataId;		// Id переменной
	uint32_t dataLenght;	// Размер данных в словах
}structData_t;

uint8_t flashEepromInit(void); // Инициализация памяти
uint8_t flashEepromWriteData(uint32_t id, uint32_t *data, uint32_t lenght); // Записать данные в EEPROM
uint8_t flashEepromDeleteData(uint32_t id); // Удалить данные из EEPROM
uint8_t flashEepromReadData(uint32_t id, uint32_t *data); // Считать данные из EEPROM

#endif
