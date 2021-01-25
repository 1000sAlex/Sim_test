/*
 * servo.c
 *
 *  Created on: Dec 7, 2020
 *      Author: u
 */

#include "servo.h"
#include "stepper.h"
#include "core.h"
#include "uart_ui.h"

Servo_str Servo;
Servo_pos_str servo_pos;
extern TIM_HandleTypeDef htim4;
extern core_str Core;

#define T 100

void Servo_task(void *args);
void Servo_work(Servo_str *serv);
u32 servo_deg_to_tim(u32 angle, u8 invert);

void Servo_init(void)
    {
    Servo.semaphore = xSemaphoreCreateBinary();
    Servo.queue = xQueueCreate(16, sizeof(Servo_pos_str));
    xTaskCreate(Servo_task, "servo", configMINIMAL_STACK_SIZE, &Servo,
	    osPriorityHigh, NULL);
    Servo.steps_per_second = 180 * SERVO_DEG_TO_RESOLUTION;
    STEPPER_EN_GPIO_Port->ODR |= STEPPER_EN_Pin; //выключаем шаговик, чтобы не дернулся
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    htim4.Instance->CCR1 = servo_deg_to_tim(0, SERVO_INVERT);
    HAL_Delay(1000);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    STEPPER_EN_GPIO_Port->ODR &= ~ STEPPER_EN_Pin;
    }

void Servo_task(void *args)
    {
    Servo_str *serv = (Servo_str*) args;
    xSemaphoreGive(serv->semaphore);
    for (;;)
	{
	if ( xSemaphoreTake(serv->semaphore, portMAX_DELAY ) == pdTRUE)
	    {
	    xQueueReceive(serv->queue, &serv->pos, portMAX_DELAY);
	    if (serv->pos.need_pos > 180)
		{
		Uart_send("servo err\n", sizeof("servo err\n"));
		xSemaphoreGive(serv->semaphore);
		}
	    else
		{
#if DEBUG_STR
		Uart_send_val("SERV_START pos = ", sizeof("SERV_START pos = "),
			serv->real_pos / 10);
#else
	Uart_send("servo start\n", sizeof("servo start\n"));
#endif
		HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
		serv->pos.need_pos = serv->pos.need_pos
			* SERVO_DEG_TO_RESOLUTION;
		while (uxSemaphoreGetCount(serv->semaphore) == 0)
		    {
		    Servo_work(serv);
		    }
		}
	    }
	}
    }

u32 servo_deg_to_tim(u32 angle, u8 invert)
    {
    if (angle > SERVO_RESOLUTION_0_TO_180)
	{
	angle = SERVO_RESOLUTION_0_TO_180;
	} //проверка на выход за границы
    if (invert != 0)
	{
	return SERVO_MAX - SERVO_VAL * angle;
	}
    return SERVO_MIN + SERVO_VAL * angle;
    }

void Servo_work(Servo_str *serv)
    {
    if (delta(serv->real_pos, serv->pos.need_pos)
	    > (Servo.steps_per_second / T))
	{
	if (serv->real_pos > serv->pos.need_pos)
	    {
	    serv->real_pos = serv->real_pos - (Servo.steps_per_second / T);
	    }
	else
	    {
	    serv->real_pos = serv->real_pos + (Servo.steps_per_second / T);
	    }
	}
    else
	{
	if (serv->real_pos > serv->pos.need_pos)
	    {
	    serv->real_pos = serv->pos.need_pos;
	    }
	else
	    {
	    serv->real_pos = serv->pos.need_pos;
	    }
	}
    htim4.Instance->CCR1 = servo_deg_to_tim(serv->real_pos, SERVO_INVERT);
    vTaskDelay(1000 / T);
    if (serv->real_pos == serv->pos.need_pos)
	{
	xSemaphoreGive(serv->semaphore);
#if DEBUG_STR
	Uart_send_val("SERV_READY pos = ", sizeof("SERV_READY pos = "),
		serv->real_pos / 10);
#else
	Uart_send("servo ready\n", sizeof("servo ready\n"));
#endif
	if (serv->pos.source == COM_FROM_CORE)
	    {
	    vTaskResume(Core.core_Handle);
	    }
	vTaskDelay(500);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
	}
    }

