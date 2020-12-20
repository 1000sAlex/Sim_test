/*
 * uart_ui.c
 *
 *  Created on: Dec 8, 2020
 *      Author: u
 */

#include "usart.h"
#include "uart_ui.h"
#include "stepper.h"
#include "servo.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "core.h"
#include "string.h"

static Stepper_pos_str stepper_pos;
extern Step_motor_str Stepper;

extern Servo_str Servo;
static Servo_pos_str servo_pos;

extern core_str Core;
static core_com_str core_com;

u8 uart_rx_buf[128];
u16 rx_count = 0;
u8 hello[] = "hello\nPrint h! to help\n";

Uart_tx_str Uart_tx;
void Uart_tx_task(void *args);
u32 itoa(s32 n, char s[]);

void uart_ui_init(void)
    {
    HAL_UART_Receive_IT(&huart1, uart_rx_buf, 128);
    HAL_UART_Transmit(&huart1, hello, sizeof(hello) - 1, 0xFFFFFFFF);
    Uart_tx.semaphore = xSemaphoreCreateBinary();
    Uart_tx.queue = xQueueCreate(8, sizeof(Uart_tx_data_str));
    xTaskCreate(Uart_tx_task, "uart_tx", configMINIMAL_STACK_SIZE, &Uart_tx,
	    osPriorityNormal, NULL);
    }

void Uart_tx_task(void *args)
    {
    Uart_tx_str *uart = (Uart_tx_str*) args;
    xSemaphoreGive(uart->semaphore);
    for (;;)
	{
	xQueueReceive(uart->queue, &uart->data, portMAX_DELAY);
	if ( xSemaphoreTake(uart->semaphore, portMAX_DELAY ) == pdTRUE)
	    {
	    HAL_UART_Transmit(&huart1, uart->data.buf, uart->data.len - 1,
		    0xFFFFFFFF);
	    xSemaphoreGive(uart->semaphore);
	    }
	}
    }

void Uart_send(char buf[], uint8_t len)
    {
    Uart_tx_data_str data;
    data.len = len;
    memcpy(data.buf, buf, len);
    xQueueSend(Uart_tx.queue, &data, portMAX_DELAY);
    }

void Uart_send_isr(char buf[], uint8_t len)
    {
    Uart_tx_data_str data;
    data.len = len;
    memcpy(data.buf, buf, len);
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(Uart_tx.queue, &data, &xHigherPriorityTaskWoken);
    }

void Uart_send_val(char buf[], uint8_t len, s32 val)
    {
    Uart_tx_data_str data;
    memcpy(data.buf, buf, len);
    u32 len_val = itoa(val, (char*) &data.buf[len - 1]);
    data.len = len + len_val;
    data.buf[data.len - 1] = '\n';
    data.len++;
    xQueueSend(Uart_tx.queue, &data, portMAX_DELAY);
    }

void Uart_send_val_isr(char buf[], uint8_t len, s32 val)
    {
    Uart_tx_data_str data;
    memcpy(data.buf, buf, len);
    u32 len_val = itoa(val, (char*) &data.buf[len - 1]);
    data.len = len + len_val;
    data.buf[data.len - 1] = '\n';
    data.len++;
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(Uart_tx.queue, &data, &xHigherPriorityTaskWoken);
    }

void uart_pars(u8 *data, u16 len)
    {
    u32 rx_val = 0;
    u32 order = 1;
    u16 l = 0; //хранит правый конец текущей команды
    u16 local_l = 0;
    u16 local_end = 0;
    u8 n_attr = 0;
    while (l < len) //идем до конца принятого буфера
	{
	if (data[l] == '!') //идем вправо до '!'
	    {
	    local_l = l;
	    while (local_l != local_end)
		{
		local_l--;
		if ((data[local_l] >= '0') && (data[local_l] <= '9'))
		    {
		    rx_val = rx_val + (order * (data[local_l] - '0'));
		    order = order * 10;
		    }
		else if ((data[local_l] == 'x') || (data[local_l] == 'X'))
		    {
		    stepper_pos.need_pos = rx_val;
		    stepper_pos.source = COM_FROM_ISR;
		    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		    xQueueSendFromISR(Stepper.queue, &stepper_pos,
			    &xHigherPriorityTaskWoken);
		    break;
		    }
		else if ((data[local_l] == 'y') || (data[local_l] == 'Y'))
		    {
		    servo_pos.need_pos = rx_val;
		    servo_pos.source = COM_FROM_ISR;
		    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		    xQueueSendFromISR(Servo.queue, &servo_pos,
			    &xHigherPriorityTaskWoken);
		    break;
		    }
		else if ((data[local_l] == ';') || data[local_l] == '=')
		    {
		    core_com.atr_buf[n_attr++] = rx_val;
		    rx_val = 0;
		    order = 1;
		    }
		else if ((data[local_l] == 'm') || (data[local_l] == 'M'))
		    {
		    if (n_attr == 0)
			{
			core_com.atr_buf[0] = rx_val;
			}
		    else
			{
			core_com.n_atr = n_attr;
			}
		    core_com.n_com = rx_val;
		    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		    xQueueSendFromISR(Core.queue, &core_com,
			    &xHigherPriorityTaskWoken);
		    break;
		    }
		else if ((data[local_l] == 'h') || (data[local_l] == 'H'))
		    {
		    Uart_send_isr("Help:\n ", sizeof("Help:\n "));
		    Uart_send_isr("command X stepper_pos !\n ",
			    sizeof("ommand X stepper_pos !\n "));
		    Uart_send_isr("command Y servo_pos !\n ",
			    sizeof("ommand Y servo_pos !\n "));
		    Uart_send_isr("command M0 = stepper_pos; servo_pos !\n ",
			    sizeof("command M0 = stepper_pos; servo_pos !\n "));
		    Uart_send_isr(
			    "command M1 = servo_pos; stepper_pos; servo_pos !\n ",
			    sizeof("command M1 = servo_pos; stepper_pos; servo_pos !\n "));
		    Uart_send_isr("command M2 = N_sim !\n ",
			    sizeof("command M2 = N_sim !\n "));
		    break;
		    }
		}
	    n_attr = 0;
	    rx_val = 0;
	    order = 1;
	    local_end = l;
	    }
	l++;
	}
    rx_count = 0;
    }

void HAL_UART_Rx_int(UART_HandleTypeDef *huart)
    {
    if (huart->Instance == USART1)
	{
	if (huart->Instance->DR != 0x0A)
	    {
//	    uart_rx_buf[rx_count] = huart->Instance->DR;
	    rx_count++;
	    }
	else
	    {
	    uart_pars(uart_rx_buf, rx_count);
	    HAL_UART_AbortReceive_IT(&huart1);
	    HAL_UART_Receive_IT(&huart1, uart_rx_buf, 128);
	    }
	}
    }

/* reverse:  переворачиваем строку s на месте */
void reverse(char s[])
    {
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
	{
	c = s[i];
	s[i] = s[j];
	s[j] = c;
	}
    }

/* itoa:  конвертируем n в символы в s */
u32 itoa(s32 n, char s[])
    {
    s32 i, sign;

    if ((sign = n) < 0) /* записываем знак */
	n = -n; /* делаем n положительным числом */
    i = 0;
    do
	{ /* генерируем цифры в обратном порядке */
	s[i++] = n % 10 + '0'; /* берем следующую цифру */
	}
    while ((n /= 10) > 0); /* удаляем */
    if (sign < 0)
	s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    return i;
    }
