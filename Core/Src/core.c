/*
 * core.c
 *
 *  Created on: Dec 17, 2020
 *      Author: u
 */

#include "core.h"
#include "stepper.h"
#include "servo.h"

void Core_task(void *args);
core_str Core;

static Stepper_pos_str stepper_pos;
extern Step_motor_str Stepper;

extern Servo_str Servo;
static Servo_pos_str servo_pos;

void core_com_pars(core_str* cor, core_com_str *com);

void core_init(void)
    {
    Core.semaphore = xSemaphoreCreateBinary();
    Core.queue = xQueueCreate(16, sizeof(core_com_str));
    xTaskCreate(Core_task, "Core", configMINIMAL_STACK_SIZE, &Core,
	    osPriorityRealtime, NULL);
    }

void Core_task(void *args)
    {
    core_str *cor = (core_str*) args;
    xSemaphoreGive(cor->semaphore);
    for (;;)
	{
	xQueueReceive(cor->queue, &cor->com, portMAX_DELAY);
	if ( xSemaphoreTake(cor->semaphore, portMAX_DELAY ) == pdTRUE)
	    {
	    core_com_pars(cor, &cor->com);
	    vTaskDelay(10);
	    }
	}
    }

void core_com_pars(core_str* cor, core_com_str *com)
    {
    switch (com->n_com)
	{
    case 0:
	if (com->n_atr == 2)
	    {
	    stepper_pos.need_pos = com->atr_buf[1];
	    servo_pos.need_pos = com->atr_buf[0];//правое значение
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    while (uxSemaphoreGetCount(Stepper.semaphore) == 0)
		;
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    while (uxSemaphoreGetCount(Servo.semaphore) == 0)
		;
	    xSemaphoreGive(cor->semaphore);
	    }
	else
	    {

	    }
	break;
    case 1:
	if (com->n_atr == 3)
	    {
	    servo_pos.need_pos = com->atr_buf[3];
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    while (uxSemaphoreGetCount(Servo.semaphore) == 0)
		;
	    stepper_pos.need_pos = com->atr_buf[1];
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    while (uxSemaphoreGetCount(Stepper.semaphore) == 0)
		;
	    servo_pos.need_pos = com->atr_buf[0];
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    while (uxSemaphoreGetCount(Servo.semaphore) == 0)
		;
	    xSemaphoreGive(cor->semaphore);
	    }
	else
	    {

	    }
	break;
    default:
#if DEBUG_STR
	Uart_send("core com err\n", sizeof("core com err\n"));
#else
	Uart_send("core com err\n", sizeof("core com err\n"));
#endif
	xSemaphoreGive(cor->semaphore);
	break;
	}
    }
