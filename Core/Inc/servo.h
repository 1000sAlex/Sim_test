/*
 * servo.h
 *
 *  Created on: Dec 7, 2020
 *      Author: u
 */

#ifndef INC_SERVO_H_
#define INC_SERVO_H_

#include "main.h"

#define SERVO_INVERT 1
#define SERVO_NON_INVERT 0


//микросекунды
#define SERVO_US_180 1850
#define SERVO_US_0 500

#define SERVO_RESOLUTION_0_TO_180 1800
#define SERVO_DEG_TO_RESOLUTION SERVO_RESOLUTION_0_TO_180/180


#define SERVO_FREQUENCY 60

#define SERVO_RESOLUTION 65535
#define SERVO_PRESCALER (F_CPU/(SERVO_RESOLUTION*SERVO_FREQUENCY))

#define SERVO_OFSET (SERVO_US_0*(F_CPU/1000000))/SERVO_PRESCALER

#define SERVO_MIN SERVO_OFSET
#define SERVO_MAX ((SERVO_US_180*(F_CPU/1000000))/SERVO_PRESCALER)
#define SERVO_VAL (SERVO_MAX - SERVO_MIN) / SERVO_RESOLUTION_0_TO_180

typedef struct servo_pos_struct
    {
	u32 need_pos;
	u8 source;
    } Servo_pos_str;

typedef struct servo_struct
    {
	xSemaphoreHandle semaphore;
	QueueHandle_t queue;
	Servo_pos_str pos; //необходимое положение
	u32 real_pos; //нынешнее положение
	u32 steps_left; //шагов осталось до цели
	u32 steps_per_second;
    } Servo_str;

    void Servo_init(void);
#endif /* INC_SERVO_H_ */
