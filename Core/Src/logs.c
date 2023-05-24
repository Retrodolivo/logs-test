#include <stdbool.h>
#include <string.h>
#include "flashMemory.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "logs.h"

Logs_st logs;

static Flash_page_st page[PAGES];
/*in total_pages do not include end page*/
static uint8_t total_pages = 0;
static uint8_t start_page = 0;
static uint8_t end_page = 0;
static const uint16_t bytes_in_page = 0x800;
static uint8_t index_page = 0;
static uint16_t offset = 0;
static void context_record(uint16_t ind, uint16_t off);
static void get_context(uint16_t *index, uint16_t *offset);

static void u32_to_u8arr(uint32_t u32, uint8_t *u8);
static void arr8_tx(uint8_t *arr, uint16_t len);


/*sets flash boundaries for log data*/
/*
 * arg[0] - put down an address of last used address of code. Function will choose the very next page for context savings(needed after reset).
 * The next from that point would be writing log data start page.
 * arg[1] - put down an end address.
 * */
void logs_init(uint32_t start_addr, uint32_t end_addr)
{


	page[0].start = 0x08000000;
	page[0].end = 0x080007FF;

	for(uint8_t i = 1; i < PAGES; i++)
	{
		page[i].start = page[i - 1].end + 1;
		page[i].end = page[i].start + bytes_in_page - 1;
	}

	/*find last used flash page by main program code. The next page would be the start of a log data storage
	 *The end of a log data storage would be the end_page.
	 * */
	for (uint8_t i = 0; i < PAGES; i++)
	{
		if (page[i].start >= start_addr)
		{
			index_page = i;
			start_page = i + 1;
			break;
		}
	}
	for (uint8_t i = 0; i < PAGES; i++)
	{
		if (page[i].end >= end_addr)
		{
			end_page = i;
			break;
		}
	}

	logs.action = NO_ACTION;
	logs.timestamp = 0;
	logs.card = 0;
	logs.cell = 0;
	/*retrieve context after reset*/
	get_context(&logs.index, &offset);

	total_pages = (end_page - start_page) == 0 ? 1 : end_page - start_page;

}


uint8_t current_page = 0;
/*Cycle writing mode. When there is no room for write new data, it's going from the start of the log data storage, erased page before that*/
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
	context_record(logs->index++, offset);
}

void logs_read(void)
{
	CDC_Transmit_FS(&total_pages, 1);
	HAL_Delay(1);
	uint16_t records_in_page = bytes_in_page / sizeof(Logs_st);

	uint32_t record32[sizeof(Logs_st) / sizeof(uint32_t)];
	memset(record32, 0, sizeof(record32) / sizeof(record32[0]));

	uint8_t record8[sizeof(uint32_t)];
	memset(record8, 0, sizeof(record8) / sizeof(record8[0]));

	for (uint8_t i = start_page; i < end_page; i++)
	{
		for (uint16_t j = 0; j < records_in_page; j++)
		{
			flashReadDataWord(page[i].start + sizeof(Logs_st) * j, record32, sizeof(record32) / sizeof(record32[0]));
			for (uint8_t k = 0; k < sizeof(Logs_st) / sizeof(uint32_t); k++)
			{
				u32_to_u8arr(record32[k], record8);
				arr8_tx(record8, sizeof(record8));
			}
		}
	}
}

static void u32_to_u8arr(uint32_t u32, uint8_t *u8)
{
	for(uint8_t i = 0; i < sizeof(u32); i++)
	{
		u8[i] = u32 >> 8 * i;
	}
}
static void arr8_tx(uint8_t *arr, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		CDC_Transmit_FS(&arr[i], 1);
		/*Delay is necessary, unfortunately :(*/
		HAL_Delay(1);
	}

}

/*duplicates value of log data index and offset to page[index_page]*/
static const uint16_t records_in_page = bytes_in_page / sizeof(uint32_t);
static void context_record(uint16_t index, uint16_t offset)
{
	static uint16_t step = 0;
	if (step == 0)
	{
		flashErasePage(page[index_page].start);
	}
	struct
	{
		uint16_t off;
		uint16_t ind;
	}context;
	context.ind = index;
	context.off = offset;

	if (step < records_in_page)
	{
		/*make records template as 'XXXXYYYY, where XXXX(upper) - uint16_t index and YYYY(lower) - uint16_t offset*/
		flashWriteDataWord(page[index_page].start + 4 * step++, (uint32_t *)&context, sizeof(context) / sizeof(uint32_t));
	}
	else
	{
		flashErasePage(page[index_page].start);
		step = 0;
		flashWriteDataWord(page[index_page].start + 4 * step++, (uint32_t *)&context, sizeof(context) / sizeof(uint32_t));
	}
}

static void get_context(uint16_t *index, uint16_t *offset)
{
	uint32_t ret = 0;
	for (uint16_t i = records_in_page - 1; i > 0; i--)
	{
		if (flashReadWord(page[index_page].start + i * sizeof(uint32_t) - 4) != 0xFFFFFFFF)
		{
			ret = (uint32_t)flashReadWord(page[index_page].start + i * sizeof(uint32_t) - 4);
			break;
		}
	}
	/*return value of the NEXT index*/
	if (ret > 0)
	{
		*index = (uint16_t)(ret >> 16) + 1;
		*offset = (uint16_t)ret;
	}
	else
	{
		*index = 0;
		*offset = 0;
	}
}
