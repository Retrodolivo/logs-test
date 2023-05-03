#ifndef _FLASHMEMORY_H
#define _FLASHMEMORY_H

#include "main.h"
#include "bitOperations.h"

uint8_t flashErasePage(uint32_t address); // Стереть страницу флеш
uint8_t flashWriteWord(uint32_t address, uint32_t data); // Запись в флеш
uint8_t flashWrite_16(uint32_t address, uint16_t data); // Запись 16 бит в флеш.
uint8_t flashRead_8(uint32_t address); // Чтение 8 бит данных из флеш
uint16_t flashRead_16(uint32_t address); // Чтение 16 бит данных из флеш
uint32_t flashReadWord(uint32_t address); // Чтение данных из флеш
void flashReadDataWord(uint32_t startAddresData, uint32_t *buffer, uint32_t lenght); // Чтение данных длинною в lenght слов  из FLASH
uint8_t flashWriteDataWord(uint32_t address, uint32_t *data, uint16_t lenght); // Запись данных длинною lenght в флеш. Данные пишутся по 16 бит

#endif
