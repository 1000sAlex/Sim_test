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

extern Step_motor_str Stepper;
extern Servo_str Servo;

void core_com_pars(core_str *cor, core_com_str *com);

void core_init(void)
    {
    Core.semaphore = xSemaphoreCreateBinary();
    Core.queue = xQueueCreate(16, sizeof(core_com_str));
    xTaskCreate(Core_task, "Core", configMINIMAL_STACK_SIZE, &Core,
	    osPriorityRealtime, &Core.core_Handle);
    }

void Core_task(void *args)
    {
    core_str *cor = (core_str*) args;
    core_com_str com;
    com.n_com = 0;
    xQueueSend(Core.queue, &com, portMAX_DELAY);
    xSemaphoreGive(cor->semaphore);
    for (;;)
	{
	xQueueReceive(cor->queue, &cor->com, portMAX_DELAY);
	if ( xSemaphoreTake(cor->semaphore, portMAX_DELAY ) == pdTRUE)
	    {
	    core_com_pars(cor, &cor->com);
	    }
	}
    }

void core_com_pars(core_str *cor, core_com_str *com)
    {
    Stepper_pos_str stepper_pos =
	{
	0
	};
    Servo_pos_str servo_pos =
	{
	0
	};
    servo_pos.source = COM_FROM_CORE;
    stepper_pos.source = COM_FROM_CORE;
    switch (com->n_com)
	{
    case 0:
	if (com->n_atr == 0)
	    {
#if DEBUG_STR
	    Uart_send("M0 Start\n", sizeof("M0 Start\n"));
#endif
	    servo_pos.need_pos = 0;
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);

	    stepper_pos.com = STEPPER_COM_ZERO;
	    stepper_pos.jerk = 150;
	    stepper_pos.acceler = 10000;
	    stepper_pos.max_speed = 4000;
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    vTaskDelay(100);
	    stepper_pos.com = 0;
	    stepper_pos.need_pos = 300;
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    vTaskDelay(100);
	    stepper_pos.com = STEPPER_COM_ZERO;
	    stepper_pos.jerk = 250;
	    stepper_pos.acceler = 0;
	    stepper_pos.max_speed = 250;
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    xSemaphoreGive(cor->semaphore);
#if DEBUG_STR
	    Uart_send("M0 Finish\n", sizeof("M0 Finish\n"));
#endif
	    }
	else
	    {
	    Uart_send("core n_atr err\n", sizeof("core n_atr err\n"));
	    xSemaphoreGive(cor->semaphore);
	    }
	break;
    case 1:
	if (com->n_atr == 2)
	    {
#if DEBUG_STR
	    Uart_send("M1 Start\n", sizeof("M1 Start\n"));
#endif
	    stepper_pos.need_pos = com->atr_buf[1];
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    servo_pos.need_pos = com->atr_buf[0]; //правое значение
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    xSemaphoreGive(cor->semaphore);

#if DEBUG_STR
	    Uart_send("M1 Finish\n", sizeof("M1 Finish\n"));
#endif
	    }
	else
	    {
	    Uart_send("core n_atr err\n", sizeof("core n_atr err\n"));
	    xSemaphoreGive(cor->semaphore);
	    }
	break;
    case 2:
	if (com->n_atr == 3)
	    {
#if DEBUG_STR
	    Uart_send("M2 Start\n", sizeof("M2 Start\n"));
#endif
	    servo_pos.need_pos = com->atr_buf[3];
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    stepper_pos.need_pos = com->atr_buf[1];
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    servo_pos.need_pos = com->atr_buf[0];
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    xSemaphoreGive(cor->semaphore);
#if DEBUG_STR
	    Uart_send("M2 Finish\n", sizeof("M2 Finish\n"));
#endif
	    }
	else
	    {
	    Uart_send("core n_atr err\n", sizeof("core n_atr err\n"));
	    xSemaphoreGive(cor->semaphore);
	    }
	break;
    case 3:
	if (com->n_atr == 1)
	    {
#if DEBUG_STR
	    Uart_send("M3 Start\n", sizeof("M3 Start\n"));
#endif
	    servo_pos.need_pos = 0;
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    stepper_pos.need_pos = com->atr_buf[0] * STEPPER_STEPS_PER_SIM;
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    servo_pos.need_pos = 180;
	    xQueueSend(Servo.queue, &servo_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    stepper_pos.need_pos = com->atr_buf[0] * STEPPER_STEPS_PER_SIM
		    - 500;
	    xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
	    vTaskSuspend(cor->core_Handle);
	    xSemaphoreGive(cor->semaphore);
#if DEBUG_STR
	    Uart_send("M3 Finish\n", sizeof("M3 Finish\n"));
#endif
	    }
	else
	    {
	    Uart_send("core n_atr err\n", sizeof("core n_atr err\n"));
	    xSemaphoreGive(cor->semaphore);
	    }
	break;
    default:
	Uart_send("core n_com err\n", sizeof("core n_com err\n"));
	xSemaphoreGive(cor->semaphore);
	break;
	}
    }
