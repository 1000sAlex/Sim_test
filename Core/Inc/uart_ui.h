/*
 * uart_ui.h
 *
 *  Created on: Dec 8, 2020
 *      Author: u
 */

#ifndef INC_UART_UI_H_
#define INC_UART_UI_H_

#include "main.h"
#include "FreeRTOS.h"

typedef struct uart_tx_data_struct
    {
	uint8_t len;
	uint8_t buf[32];
    } Uart_tx_data_str;

typedef struct uart_tx_struct
    {
	SemaphoreHandle_t semaphore;
	QueueHandle_t queue;
	Uart_tx_data_str data;
    } Uart_tx_str;

void uart_ui_init(void);
void HAL_UART_Rx_int(UART_HandleTypeDef *huart);
void Uart_send(char buf[], uint8_t len);
void Uart_send_val(char buf[], uint8_t len, int32_t val);
void Uart_send_isr(char buf[], uint8_t len);
void Uart_send_val_isr(char buf[], uint8_t len, int32_t val);

#endif /* INC_UART_UI_H_ */
