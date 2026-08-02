/**
 * @file    UartUtils.h
 * @brief   Lightweight UART logging/TX library for STM32F103C8T6 (HAL).
 *
 * Features
 * --------
 *  - UartUtils_Print()    – send a raw string
 *  - UartUtils_PrintLn()  – send a string + "\r\n"
 *  - UartUtils_Printf()   – printf-style formatted output (no heap, stack buffer)
 *  - UartUtils_PrintHex() – dump a byte array as hex
 *  - All calls are blocking (polling); safe to call from main loop or faults.
 *  - Timeout-guarded so a broken UART never hangs the MCU.
 *
 * Usage
 * -----
 *  UartUtilsHandle uart;
 *  UartUtils_Init(&uart, &huart2);
 *  UartUtils_Printf(&uart, "Hello %s\r\n", "world");
 */

#ifndef UART_UTILS_H
#define UART_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdarg.h>

/* Maximum formatted string length (stack-allocated). Increase if needed. */
#ifndef UART_UTILS_BUF_SIZE
#define UART_UTILS_BUF_SIZE 256
#endif

/* Per-byte transmit timeout in ms. */
#ifndef UART_UTILS_TX_TIMEOUT_MS
#define UART_UTILS_TX_TIMEOUT_MS 10
#endif

typedef struct {
    UART_HandleTypeDef *huart;
} UartUtilsHandle;

/**
 * @brief  Bind the library to a HAL UART handle.
 * @param  handle  Library handle to initialise.
 * @param  huart   Pointer to an already-initialised HAL UART handle.
 */
void UartUtils_Init(UartUtilsHandle *handle, UART_HandleTypeDef *huart);

/**
 * @brief  Transmit a null-terminated string (no newline appended).
 */
void UartUtils_Print(UartUtilsHandle *handle, const char *str);

/**
 * @brief  Transmit a null-terminated string followed by "\r\n".
 */
void UartUtils_PrintLn(UartUtilsHandle *handle, const char *str);

/**
 * @brief  printf-style formatted output.
 *         Output is truncated to UART_UTILS_BUF_SIZE - 1 characters.
 */
void UartUtils_Printf(UartUtilsHandle *handle, const char *fmt, ...);

/**
 * @brief  Dump a byte array as space-separated uppercase hex, followed by "\r\n".
 * @param  label  Optional prefix string (pass "" for none).
 * @param  data   Pointer to data bytes.
 * @param  len    Number of bytes to dump.
 */
void UartUtils_PrintHex(UartUtilsHandle *handle,
                        const char *label,
                        const uint8_t *data,
                        uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* UART_UTILS_H */
