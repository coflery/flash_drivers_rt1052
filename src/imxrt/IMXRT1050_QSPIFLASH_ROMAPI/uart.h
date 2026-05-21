#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void BOARD_InitDebugConsole(void);
void debug_printf(const char *fmt, ...);
void debug_printfn(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif