#include "uart.h"
#include "MIMXRT1052.h"
#include <stdarg.h>

#define UART_CLOCK_HZ   80000000U
#define UART_BAUDRATE   921600U

static volatile LPUART_Type *const uart = LPUART1;

static inline void UART_SetTxRxEnabled(uint8_t enable)
{
    if (enable)
        uart->CTRL |= LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;
    else
        uart->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);
}

static void CalcBaudParams(uint32_t baudrate, uint8_t *osr, uint16_t *sbr);

/* Calculate best OSR/SBR for given baudrate @ 80MHz
 * OSR range: 4-32 (higher = more tolerant but lower max baud)
 * Try to find OSR where error < 3% */
static void CalcBaudParams(uint32_t baudrate, uint8_t *osr, uint16_t *sbr)
{
    uint32_t bestErr = 0xFFFFFFFF;
    uint8_t bestOsr = 16;
    uint16_t bestSbr = 1;

    for (uint8_t o = 4; o <= 32; o++)
    {
        uint16_t s = (uint16_t)((UART_CLOCK_HZ + (o * baudrate / 2)) / (o * baudrate));
        if (s == 0) s = 1;
        if (s > 0x1FFF) s = 0x1FFF;

        uint32_t actual = UART_CLOCK_HZ / (o * s);
        uint32_t err = (actual > baudrate) ? (actual - baudrate) : (baudrate - actual);

        if (err < bestErr)
        {
            bestErr = err;
            bestOsr = o;
            bestSbr = s;
        }
    }

    *osr = bestOsr;
    *sbr = bestSbr;
}

void BOARD_SetUartBaudrate(uint32_t baudrate)
{
    UART_SetTxRxEnabled(0);

    /* Calculate and set new baud rate */
    uint8_t osr;
    uint16_t sbr;
    CalcBaudParams(baudrate, &osr, &sbr);

    uint32_t baudReg = LPUART_BAUD_SBR(sbr) |
                       LPUART_BAUD_OSR(osr - 1);
    if (osr >= 4 && osr <= 7)
        baudReg |= LPUART_BAUD_BOTHEDGE_MASK;

    uart->BAUD = baudReg;

    UART_SetTxRxEnabled(1);
}

void BOARD_InitDebugConsole(void)
{
    /* Enable clock */
    CCM->CCGR5 |= CCM_CCGR5_CG12(3);

    /* Reset peripheral */
    uart->GLOBAL = LPUART_GLOBAL_RST_MASK;
    uart->GLOBAL = 0;

    UART_SetTxRxEnabled(0);

    /* Set baud rate */
    uint8_t osr;
    uint16_t sbr;
    CalcBaudParams(UART_BAUDRATE, &osr, &sbr);
    uint32_t baudReg = LPUART_BAUD_SBR(sbr) |
                       LPUART_BAUD_OSR(osr - 1);
    if (osr >= 4 && osr <= 7)
        baudReg |= LPUART_BAUD_BOTHEDGE_MASK;
    uart->BAUD = baudReg;

    /* 8 data bits, no parity (default) */
    uart->CTRL &= ~(LPUART_CTRL_M_MASK | LPUART_CTRL_PE_MASK);

    UART_SetTxRxEnabled(1);
}

void debug_printf(const char *fmt, ...)
{
    static char buf[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    for (int i = 0; i < n && i < (int)sizeof(buf); i++)
    {
        while (!(uart->STAT & LPUART_STAT_TDRE_MASK));
        uart->DATA = (uint8_t)buf[i];
    }
}

void debug_printfn(const char *fmt, ...)
{
    static char buf[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    for (int i = 0; i < n && i < (int)sizeof(buf); i++)
    {
        while (!(uart->STAT & LPUART_STAT_TDRE_MASK));
        uart->DATA = (uint8_t)buf[i];
    }
    while (!(uart->STAT & LPUART_STAT_TDRE_MASK));
    uart->DATA = '\r';
    while (!(uart->STAT & LPUART_STAT_TDRE_MASK));
    uart->DATA = '\n';
}
