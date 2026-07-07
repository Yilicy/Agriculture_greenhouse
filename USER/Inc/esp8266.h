#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"

#define RX_BUFFER_SIZE 512
#define SERVER_IP "10.17.102.190"
#define SERVER_PORT 5000

typedef enum {
    ESP_OK = 0,
    ESP_FAIL = 1,
    ESP_TIMEOUT = 2
} ESP_Status_t;

typedef enum {
    MSG_UPLOAD_DATA,
    MSG_PROCESS_REQUEST
} EspMessageType_t;

typedef struct {
    EspMessageType_t type;
} EspMessage_t;

extern volatile uint16_t USART3_RX_STA;
extern char USART3_RX_BUF[RX_BUFFER_SIZE];
extern QueueHandle_t espQueueHandle;
extern SemaphoreHandle_t dma_tx_done;

void ESP8266_UART_IRQHandler(void);
ESP_Status_t ESP8266_SendCmd(const char *cmd, const char *ack, uint32_t timeout);
ESP_Status_t ESP8266_Init(const char *ssid, const char *password);
ESP_Status_t ESP8266_GetIP(char *ip_buffer);
ESP_Status_t ESP8266_StartServer(uint16_t port);
void ESP8266_ProcessRequest(void);
void ControlDevice(const char *device, uint8_t action);
void upload_sensor_data(void);
void RequestUpload(void);

#endif
