#ifndef INC_LOGS_H_
#define INC_LOGS_H_

#include <stdint.h>


#define PAGES	128 //for stm32f303vct6 at least

typedef enum __attribute__ ((__packed__))
{
	NO_ACTION = 1,
	LOCK_IDLE = 2,
	UNLOCK_IDLE = 3,
	LOCK_CELL = 4,
	UNLOCK_CELL = 5
} Action_et;

typedef struct
{
	uint32_t timestamp;
	uint32_t card;
	uint16_t index;
	Action_et action;
	uint8_t cell;
} Logs_st;

typedef struct
{
	uint32_t start;
	uint32_t end;
} Flash_page_st;


void logs_init(uint32_t start_addr, uint32_t end_addr);
void logs_write(Logs_st *logs);
void logs_read(void);

#endif /* INC_LOGS_H_ */
