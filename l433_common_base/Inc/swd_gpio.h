#ifndef SWD_GPIO_H
#define SWD_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "stm32l4xx.h"

/* ========================================================= */
/* Pin Definition                                            */
/* ========================================================= */

#define SWCLK_PORT         GPIOA
#define SWCLK_PIN          5

#define SWDIO_PORT         GPIOA
#define SWDIO_PIN          6

#define NRST_PORT          GPIOA
#define NRST_PIN           7


/* ========================================================= */
/* API                                                       */
/* ========================================================= */

void SWD_GPIO_Init(void);

void SWCLK_High(void);
void SWCLK_Low(void);

void SWDIO_High(void);
void SWDIO_Low(void);

void SWDIO_Output(void);
void SWDIO_Input(void);

uint32_t SWDIO_Read(void);

void SWD_Target_Reset(bool state);


/* ========================================================= */
/* Delay                                                     */
/* ========================================================= */

static inline void SWD_Delay(void)
{
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}


#ifdef __cplusplus
}
#endif

#endif