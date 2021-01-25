/*
 * sim800.c
 *
 *  Created on: Jan 19, 2021
 *      Author: u
 */

#include "sim800.h"
#include "gsm.h"

Uart_tx_str Uart_sim_tx;
void task_gsm(void *argument);
void task_other(void *argument);
//extern UART_HandleTypeDef huart3;

u8 uart3_rx_buf[128];

void uart_sim_init(void)
    {
//    HAL_UART_Receive_IT(&huart3, uart3_rx_buf, 128);
    Uart_sim_tx.semaphore = xSemaphoreCreateBinary();
    Uart_sim_tx.queue = xQueueCreate(2, sizeof(Uart_tx_data_str));
    xTaskCreate(task_gsm, "gsm_loop", 4*configMINIMAL_STACK_SIZE,
	    &Uart_sim_tx, osPriorityNormal, NULL);
    }

void task_gsm(void *argument)
    {
    gsm_init();
    gsm_power(true);
    while (1)
	{
	gsm_loop();
	}
    }

void task_other(void *argument)
    {
//    gsm_waitForRegister(30);
//    gsm_msg_send("+98xxxxxxx", "TEST MSG 1");
//    while (1)
//	{
//	osDelay(10000);
//	}
    }

