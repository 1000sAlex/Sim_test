/*
 * stepper.h
 *
 *  Created on: Dec 7, 2020
 *      Author: u
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "main.h"
#include "FreeRTOS.h"

#define STEPPER_COM_ZERO 1
#define STEPPER_STEPS_PER_SIM 100
typedef struct pos_struct
    {
	u32 need_pos;
	u8 source;
	u8 com;
	u32 max_speed;
	u32 acceler;
	u32 jerk;
    } Stepper_pos_str;

typedef struct stepper_speeed_struct
    {
	u32 tim_devider;
	u32 tim_devider_step;
	u32 tim_devider_jerk;
	u32 tim_devider_min_devider;
	u32 s_accel;
	u32 tim_devider_max_speed;
    } stepper_speeed_str;

#define STEPPER_LEFT 1
#define STEPPER_RIGHT 0

typedef struct stepper_struct
    {
	SemaphoreHandle_t semaphore;
	QueueHandle_t queue;
	u32 real_pos; //нынешнее положение
	u8 steps_dir;//направление вращения
	u32 steps_left; //шагов осталось до цели
	Stepper_pos_str pos; //необходимое положение
	stepper_speeed_str speed;
    } Step_motor_str;

void Stepper_init(void);
void Stepper_tim_interrupt_handler(TIM_HandleTypeDef *htim, Step_motor_str *stp);
u32 delta(u32 a, u32 b);

#endif /* INC_STEPPER_H_ */
