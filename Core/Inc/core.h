/*
 * core.h
 *
 *  Created on: Dec 17, 2020
 *      Author: u
 */

#ifndef INC_CORE_H_
#define INC_CORE_H_

#include "main.h"


typedef struct core_com_struct
    {
    u32 n_com;
    u32 n_atr;
    u32 atr_buf[16];
    } core_com_str;


typedef struct core_struct
    {
	SemaphoreHandle_t semaphore;
	QueueHandle_t queue;
	core_com_str com;
	TaskHandle_t core_Handle;
    } core_str;

void core_init(void);
#endif /* INC_CORE_H_ */
