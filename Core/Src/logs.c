#include <stdbool.h>
#include <string.h>
#include "flashMemory.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "logs.h"

Logs_st logs;

Flash_page_st page[PAGES];
uint8_t start_page = 0;
uint8_t end_page = 0;
uint16_t bytes_in_page = 0x800;

/*sets flash boundaries for log data*/
/*
 * arg[0] - put down an address of last used address of code. Function will choose the very next page for writing log data
 * arg[1] - put down an address. Function will choose the previous page.
 * */
uint8_t logs_init(uint32_t start_addr, uint32_t end_addr)
{
	uint8_t total_pages = 0;

	logs.action = NO_ACTION;
	logs.timestamp = 0;
	logs.card = 0;
	logs.cell = 0;
	logs.index = 0;

	page[0].start = 0x08000000;
	page[0].end = 0x080007FF;

	for(uint8_t i = 1; i < PAGES; i++)
	{
		page[i].start = page[i - 1].end + 1;
		page[i].end = page[i].start + bytes_in_page - 1;
	}

	/*find last used flash page by code. The next page would be the start of a log data storage
	 *The end of a log data storage would be the end_page.
	 * */
	for (uint8_t i = 0; i < PAGES; i++)
	{
		if (page[i].start >= start_addr)
		{
			start_page = i;
			break;
		}
	}
	for (uint8_t i = 0; i < PAGES; i++)
	{
		if(page[i].end >= end_addr)
		{
			end_page = i;
			break;
		}
	}
	total_pages = (end_page - start_page) == 0 ? 1 : end_page - start_page;
	return total_pages;
}

uint32_t offset = 0;
uint8_t current_page = 0;

/*Cycle writing mode. When there is no room for write new data, it's going from the start of the log data storage erased page before that*/
void logs_write(Logs_st *logs)
{
	uint32_t data_start = page[start_page].start + (sizeof(Logs_st) * logs->index) + offset;
	uint32_t data_end = data_start + sizeof(Logs_st) - 1;
	/*find current page via start address of the data*/
	for (uint8_t i = start_page; i < end_page; i++)
	{
		if (page[i].start <= data_start && page[i].end > data_start)
		{
			current_page = i;
			break;
		}
	}
	/*check if new data is going to partially overlap to the next page
	 * if so, write full data at the start of the new page
	 */
	if (page[current_page].end < data_end)
	{
		/*erase next page*/
		flashErasePage(page[current_page + 1].start);

		/*if current page is last - prepare for overwrite cycle*/
		if (current_page == end_page - 1)
		{
			offset = 0;
			logs->index = 0;
			data_start = page[start_page].start + (sizeof(Logs_st) * logs->index) + offset;
			flashErasePage(page[start_page].start);
		}
		/*else make offset of log data start to the next page*/
		else if (current_page != end_page - 1)
		{
			offset += (page[current_page + 1].start - data_start);
			data_start = page[start_page].start + (sizeof(Logs_st) * logs->index) + offset;
		}

	}
	flashWriteDataWord(data_start, (uint32_t *)logs, sizeof(Logs_st) / sizeof(uint32_t));
	logs->index++;
}


//flashReadDataWord(ADDR_PAGE_MASTER_DATA, (uint32_t *)&masterData, sizeof(masterData) / sizeof(uint32_t));
void logs_read(void)
{
	uint16_t records_in_page = bytes_in_page / sizeof(Logs_st);
	uint8_t record[sizeof(Logs_st)];
	memset(record, 0, sizeof(Logs_st));

	for (uint8_t i = start_page; i < end_page; i++)
	{
		for (uint16_t j = 0; j < records_in_page; j++)
		{
			for (uint8_t k = 0; k < sizeof(Logs_st); k++)
			{
				record[k] = flashRead_8(page[i].start + j * sizeof(Logs_st) + k);
			}
			CDC_Transmit_FS(record, sizeof(record));
		}
	}
}




