/**
 * @file    UartUtils.c
 * @brief   Lightweight UART logging/TX library for STM32F103C8T6 (HAL).
 */

#include "UartUtils.h"
#include <string.h>
#include <stdio.h>

#ifdef HAL_UART_MODULE_ENABLED
void UartUtils_Init(UartUtilsHandle *handle, UART_HandleTypeDef *huart)
{
    if (handle == NULL) return;
    handle->huart = huart;
}

void UartUtils_Print(UartUtilsHandle *handle, const char *str)
{
    if (handle == NULL || handle->huart == NULL || str == NULL) return;
    uint16_t len = (uint16_t)strlen(str);
    if (len == 0) return;
    HAL_UART_Transmit(handle->huart,
                      (uint8_t *)str,
                      len,
                      UART_UTILS_TX_TIMEOUT_MS * len);
}

void UartUtils_PrintLn(UartUtilsHandle *handle, const char *str)
{
    UartUtils_Print(handle, str);
    UartUtils_Print(handle, "\r\n");
}

void UartUtils_Printf(UartUtilsHandle *handle, const char *fmt, ...)
{
    if (handle == NULL || handle->huart == NULL || fmt == NULL) return;

    char buf[UART_UTILS_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) return;
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;

    HAL_UART_Transmit(handle->huart,
                      (uint8_t *)buf,
                      (uint16_t)len,
                      UART_UTILS_TX_TIMEOUT_MS * (uint16_t)len);
}

void UartUtils_PrintHex(UartUtilsHandle *handle,
                        const char *label,
                        const uint8_t *data,
                        uint8_t len)
{
    if (handle == NULL || handle->huart == NULL || data == NULL) return;

    if (label != NULL && label[0] != '\0') {
        UartUtils_Print(handle, label);
        UartUtils_Print(handle, " ");
    }

    char byte_str[4]; /* "XX " + null */
    for (uint8_t i = 0; i < len; i++) {
        snprintf(byte_str, sizeof(byte_str), "%02X ", data[i]);
        HAL_UART_Transmit(handle->huart,
                          (uint8_t *)byte_str, 3,
                          UART_UTILS_TX_TIMEOUT_MS * 3);
    }
    UartUtils_Print(handle, "\r\n");
}
#endif
