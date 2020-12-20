/*
 * stepper.c
 *
 *  Created on: Dec 7, 2020
 *      Author: u
 */

#include "stepper.h"
#include "semphr.h"
#include "core.h"

Step_motor_str Stepper;
Stepper_pos_str stepper_pos;

extern core_str Core;

void Stepper_task(void *args);
void make_step(u32 steps, u8 dir, Step_motor_str *stp);
void Stepper_acceleration_calc(Step_motor_str *stp);
void Stepper_acceleration_work(Step_motor_str *stp);
void Debug_str_servo_start(void);
void Debug_str_servo_ready(void);
extern TIM_HandleTypeDef htim2;

void Stepper_init(void)
    {
    Stepper.speed.jerk = 150;
    Stepper.speed.acceler = 20000;
    Stepper.speed.max_speed = 10000;

    Stepper.semaphore = xSemaphoreCreateBinary();
    Stepper.queue = xQueueCreate(16, sizeof(Stepper_pos_str));
    xTaskCreate(Stepper_task, "stepper", configMINIMAL_STACK_SIZE, &Stepper,
	    osPriorityHigh, NULL);
    STEPPER_MS0_GPIO_Port->ODR |= STEPPER_MS0_Pin;
    STEPPER_MS1_GPIO_Port->ODR |= STEPPER_MS1_Pin;
    STEPPER_MS2_GPIO_Port->ODR |= STEPPER_MS2_Pin;
    STEPPER_EN_GPIO_Port->ODR &= ~ STEPPER_EN_Pin;
    //STEPPER_EN_GPIO_Port->ODR |=  STEPPER_EN_Pin;
    }

void Stepper_task(void *args)
    {
    // xQueueSend(Stepper.queue, &stepper_pos, portMAX_DELAY);
    Step_motor_str *stp = (Step_motor_str*) args;
    xSemaphoreGive(stp->semaphore);
    for (;;)
	{
	if ( xSemaphoreTake(stp->semaphore, portMAX_DELAY ) == pdTRUE)
	    {
	    xQueueReceive(stp->queue, &stp->pos, portMAX_DELAY);
	    if (stp->pos.need_pos < stp->real_pos) //если положение левее чем надо
		{
		Debug_str_servo_start();
		Stepper_acceleration_calc(stp);
		make_step(stp->real_pos - stp->pos.need_pos,
		STEPPER_LEFT, stp);
		}
	    else if (stp->pos.need_pos > stp->real_pos) //если положение левее чем надо
		{
		Debug_str_servo_start();
		Stepper_acceleration_calc(stp);
		make_step(stp->pos.need_pos - stp->real_pos,
		STEPPER_RIGHT, stp);
		}
	    else
		{
		Debug_str_servo_start();
		xSemaphoreGive(stp->semaphore);
		if (stp->pos.source == COM_FROM_CORE)
		    {
		    vTaskResume(Core.core_Handle);
		    }
		}
	    while (uxSemaphoreGetCount(stp->semaphore) == 0)
		{
		Stepper_acceleration_work(stp);
		}

	    }
	}
    }

/*         2    2
 *     ( Vk - V0 )
 * s = -----------
 *          2a
 */
u32 accel_s_calc(u32 vk, u32 v0, u32 a)
    {
    return ((vk * vk) - (v0 * v0)) / (a * 2);
    }

u32 calc_devider(u32 speed)
    {
    return (F_CPU / 100) / speed;
    }

#define devider_calc(x) ((STEPS_PER_SEC_MAX)/x)
volatile u16 devider = 0;
volatile u32 steps_per_second = 0;
void Stepper_acceleration_calc(Step_motor_str *stp)
    {
    //запоминание рывка
    stp->speed.tim_devider_jerk = calc_devider(stp->speed.jerk);
    //рассчет минимального делителя таймера
    stp->speed.tim_devider_min_devider = calc_devider(stp->speed.max_speed);
    //рассчет пути на котором будем ускоряться
    stp->speed.s_accel = accel_s_calc(stp->speed.max_speed, stp->speed.jerk,
	    stp->speed.acceler);
    //передаем в таймер значение рывка
    htim2.Instance->PSC = stp->speed.tim_devider_jerk;
    devider = htim2.Instance->PSC;
    steps_per_second = stp->speed.jerk;
    }

u32 delta(u32 a, u32 b)
    {
    if (a >= b)
	{
	return (a - b);
	}
    else
	{
	return (b - a);
	}
    }

void Stepper_acceleration_work(Step_motor_str *stp)
    {
    if (stp->steps_left > (delta(stp->real_pos, stp->pos.need_pos) / 2))
	{
//	if (stp->steps_left < stp->speed.s_accel)   //если еще можем разгоняться
//	    {
	//разгон
	if (htim2.Instance->PSC > stp->speed.tim_devider_min_devider)
	    {
	    steps_per_second = (steps_per_second * 100 + stp->speed.acceler)
		    / 100;
	    htim2.Instance->PSC = calc_devider(steps_per_second);
	    devider = htim2.Instance->PSC;
	    if (htim2.Instance->PSC < stp->speed.tim_devider_min_devider)
		{
		htim2.Instance->PSC = stp->speed.tim_devider_min_devider;
		}
	    }
//	    }
	}
    else    //если тормозим
	{
	if (stp->steps_left < stp->speed.s_accel)   //если надо тормозить
	    {
	    //торможение
	    if (htim2.Instance->PSC < stp->speed.tim_devider_jerk)
		{
		steps_per_second = (steps_per_second * 100 - stp->speed.acceler)
			/ 100;
		htim2.Instance->PSC = calc_devider(steps_per_second);
		}
	    }
	}
    vTaskDelay(10);
    }

void make_step(u32 steps, u8 dir, Step_motor_str *stp)
    {
    if (dir != STEPPER_RIGHT)
	{
	STEPPER_DIR_GPIO_Port->ODR |= STEPPER_DIR_Pin;
	//stp->real_pos = stp->real_pos - steps;
	}
    else
	{
	STEPPER_DIR_GPIO_Port->ODR &= ~ STEPPER_DIR_Pin;
	//stp->real_pos = stp->real_pos + steps;
	}
    stp->steps_left = steps;
    HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_2);
    }

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
    {
    Stepper_tim_interrupt_handler(htim, &Stepper);
    }

void Stepper_tim_interrupt_handler(TIM_HandleTypeDef *htim, Step_motor_str *stp)
    {
    if (htim->Instance == TIM2)
	{
	if (stp->steps_left == 0)
	    {
	    HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_2);
	    stp->real_pos = stp->pos.need_pos;
	    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	    Debug_str_servo_ready();
	    xSemaphoreGiveFromISR(stp->semaphore, &xHigherPriorityTaskWoken);
	    if (stp->pos.source == COM_FROM_CORE)
		{
		xTaskResumeFromISR(Core.core_Handle);
		}
	    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	    }
	else
	    {
	    stp->steps_left--;
	    }
	}
    }

void Debug_str_servo_start(void)
    {
#if DEBUG_STR
    Uart_send_val("STEP_START pos = ", sizeof("STEP_START pos = "),
	    Stepper.real_pos);
#else
	Uart_send("step start\n", sizeof("step start\n"));
#endif
    }

void Debug_str_servo_ready(void)
    {
#if DEBUG_STR
    Uart_send_val_isr("STEP_READY pos = ", sizeof("STEP_READY pos = "),
	    Stepper.real_pos);
#else
	Uart_send("step ready\n", sizeof("step ready\n"));
#endif
    }
