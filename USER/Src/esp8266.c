#include "esp8266.h"
#include "sensor_data.h"
#include "system_config.h"
#include "SG90.h"
#include <stdlib.h>
#include "motor.h"

volatile uint16_t USART3_RX_STA = 0;
char USART3_RX_BUF[RX_BUFFER_SIZE];
QueueHandle_t espQueueHandle = NULL;
SemaphoreHandle_t dma_tx_done = NULL;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart3 && dma_tx_done != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(dma_tx_done, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void ESP8266_UART_IRQHandler(void)
{
    uint8_t ch;
    if (USART3->SR & (1 << 5))
    {
        ch = (uint8_t)(USART3->DR & 0xFF);
        uint16_t index = USART3_RX_STA & 0x3FFF;
        if (index < RX_BUFFER_SIZE - 1)
        {
            USART3_RX_BUF[index] = ch;
            USART3_RX_STA++;
        }
        if (ch == 0x0A)
        {
            USART3_RX_STA |= 0x8000;
        }
    }
}

ESP_Status_t ESP8266_GetIP(char *ip_buffer)
{
    USART3_RX_STA = 0;
    memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
    HAL_UART_Transmit(&huart3, (uint8_t*)"AT+CIFSR\r\n", strlen("AT+CIFSR\r\n"), 500);
    
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 1000)
    {
        if (USART3_RX_STA & 0x8000)
        {
            char *p = strstr(USART3_RX_BUF, "STAIP,\"");
            if (p != NULL) {
                p += 7;
                char *end = strchr(p, '"');
                if (end != NULL) {
                    int len = end - p;
                    if (len < 16) {
                        strncpy(ip_buffer, p, len);
                        ip_buffer[len] = '\0';
                        return ESP_OK;
                    }
                }
            }
            return ESP_FAIL;
        }
        HAL_Delay(10);
    }
    return ESP_FAIL;
}

ESP_Status_t ESP8266_SendCmd(const char *cmd, const char *ack, uint32_t timeout)
{
    USART3_RX_STA = 0;
    memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
    
    while (USART3->SR & (1 << 5))
    {
        volatile uint8_t dummy = USART3->DR & 0xFF;
        (void)dummy;
    }
    
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), timeout);
    
    if (ack == NULL) return ESP_OK;
    
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout)
    {
        if (USART3_RX_STA & 0x8000)
        {
            if (strstr(USART3_RX_BUF, ack) != NULL) return ESP_OK;
            USART3_RX_STA = 0;
        }
        HAL_Delay(10);
    }
    
    return ESP_TIMEOUT;
}

ESP_Status_t ESP8266_Init(const char *ssid, const char *password)
{
    char ip[16];
    
    for (int i = 0; i < 3; i++)
    {
        if (ESP8266_SendCmd("AT\r\n", "OK", 500) == ESP_OK) break;
        if (i == 2) { printf("[ESP8266] 无响应\r\n"); return ESP_FAIL; }
        HAL_Delay(200);
    }
    
    if (ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK", 500) != ESP_OK) return ESP_FAIL;
    if (ESP8266_SendCmd("AT+CIPMUX=1\r\n", "OK", 500) != ESP_OK) return ESP_FAIL;
    
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (ESP8266_SendCmd(cmd, "OK", 10000) != ESP_OK) { printf("[ESP8266] WiFi失败\r\n"); return ESP_FAIL; }
    
    if (ESP8266_GetIP(ip) == ESP_OK) printf("IP: %s\r\n", ip);
    return ESP_OK;
}

ESP_Status_t ESP8266_StartServer(uint16_t port)
{
    char cmd[64];
    sprintf(cmd, "AT+CIPSERVER=1,%d\r\n", port);
    if (ESP8266_SendCmd(cmd, "OK", 2000) != ESP_OK) return ESP_FAIL;
    printf("[SERVER] 端口:%d\r\n", port);
    return ESP_OK;
}

void ESP8266_ProcessRequest(void)
{
    if (!(USART3_RX_STA & 0x8000)) return;
    
    char *p = strstr(USART3_RX_BUF, "+IPD,");
    if (!p || !strstr(p, "GET /control")) { USART3_RX_STA = 0; memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE); return; }
    
    printf("[CTRL] %s\r\n", USART3_RX_BUF);
    
    // 解析 link_id
    int link_id = 0;
    sscanf(p, "+IPD,%d,", &link_id);
    
    // 解析 device 和 action
    char device[16] = {0}, action[8] = {0};
    char *dev = strstr(p, "device=");
    if (dev) { dev += 7; char *e = strpbrk(dev, "& \r\n"); if (e) { int l = e - dev; if (l > 0 && l < 16) { strncpy(device, dev, l); device[l] = 0; } } }
    char *act = strstr(p, "action=");
    if (act) { act += 7; char *e = strpbrk(act, "& \r\n"); if (e) { int l = e - act; if (l > 0 && l < 8) { strncpy(action, act, l); action[l] = 0; } } }
    
    printf("[CTRL] %s=%s\r\n", device, action);
    ControlDevice(device, atoi(action));
    
    // 回复 200 OK
    char body[] = "{\"status\":\"ok\"}";
    char http_response[256];
    sprintf(http_response,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", (int)strlen(body), body);
    
    int total_len = strlen(http_response);
    char cmd[64];
    sprintf(cmd, "AT+CIPSEND=%d,%d\r\n", link_id, total_len);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(100);
    HAL_UART_Transmit(&huart3, (uint8_t*)http_response, total_len, 1000);
    HAL_Delay(100);
    
    sprintf(cmd, "AT+CIPCLOSE=%d\r\n", link_id);
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    
    printf("[CTRL] 已回复OK\r\n");
    USART3_RX_STA = 0;
    memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
}

void ControlDevice(const char *device, uint8_t action)
{
    if (strcmp(device, "fan") == 0) {
        // 前端控制风扇开关
        Remote_SetFan(action);
    }
    else if (strcmp(device, "curtain") == 0) {
        // 前端控制卷帘开关
        Manual_SetCurtain(action);
    }
    else if (strcmp(device, "mode") == 0) {
        // 前端切换系统模式
        if (action == 1) {
            Manual_SetSystemAuto();
        } else {
            Manual_SetSystemManual();
        }
    }
}

void upload_sensor_data(uint8_t fla)
{
    static uint8_t first = 1;
    char json[512];
    
    if (first == 1 || fla == 1)
    {
        sprintf(json, "{\"temp\":%.1f,\"humid\":%d,\"light\":%.1f,\"mode\":%d,\"fan\":%d,\"curtain\":%d}",
            g_sensor_data.temperature, g_sensor_data.humidity, g_sensor_data.light,
            g_sys_config.system_mode, g_sys_config.fan_state, g_sys_config.curtain_state);
        first = 0;
    }
    else
    {
        sprintf(json, "{\"temp\":%.1f,\"humid\":%d,\"light\":%.1f}",
            g_sensor_data.temperature, g_sensor_data.humidity, g_sensor_data.light);
    }
    printf("[UPLOAD] JSON: %s\r\n", json);

    // TCP连接
    ESP8266_SendCmd("AT+CIPCLOSE=4\r\n", NULL, 500);
    HAL_Delay(200);
    
    char cmd[128];
    sprintf(cmd, "AT+CIPSTART=4,\"TCP\",\"%s\",%d\r\n", SERVER_IP, SERVER_PORT);
    if (ESP8266_SendCmd(cmd, "OK", 5000) != ESP_OK) 
    {
        printf("[UPLOAD] TCP连接失败\r\n");
        return;
    }
    
    // 构建HTTP请求
    char http_req[512];
    sprintf(http_req,
        "POST /api/hardware/init HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", SERVER_IP, SERVER_PORT, (int)strlen(json), json);
    
    // 发送
    int len = strlen(http_req);
    sprintf(cmd, "AT+CIPSEND=4,%d\r\n", len);
    
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 500);
    HAL_Delay(50);
    HAL_UART_Transmit(&huart3, (uint8_t*)http_req, len, 2000);
    HAL_Delay(50);
    
    printf("[UPLOAD] %s\r\n", json);
    
    HAL_UART_Transmit(&huart3, (uint8_t*)"AT+CIPCLOSE=4\r\n", 14, 500);
    
    // USART3_RX_STA = 0;
    // memset(USART3_RX_BUF, 0, RX_BUFFER_SIZE);
    // HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 1000);
    
    // uint32_t t = HAL_GetTick();
    // while (HAL_GetTick() - t < 2000)
    // {
    //     ESP8266_ProcessRequest();
    //     if (USART3_RX_STA & 0x8000 && strstr(USART3_RX_BUF, ">")) break;
    //     HAL_Delay(10);
    // }
    
    // HAL_UART_Transmit(&huart3, (uint8_t*)http_req, len, 3000);
    // HAL_Delay(200);
    
    // printf("[UPLOAD] 发送完成\r\n");
    
    // ESP8266_SendCmd("AT+CIPCLOSE=4\r\n", NULL, 500);
}

void RequestUpload(void)
{
    EspMessage_t msg;
    msg.type = MSG_UPLOAD_DATA;
    xQueueSend(espQueueHandle, &msg, 0);
}
