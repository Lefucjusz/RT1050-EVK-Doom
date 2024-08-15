/*
 * systick.c
 *
 *  Created on: Aug 13, 2024
 *      Author: lefucjusz
 */

#include "systick.h"
#include "fsl_common.h"
#include <stdio.h>

static volatile uint32_t sysTick;

void SysTick_Init(void)
{
	sysTick = 0;

	if (SysTick_Config(SystemCoreClock / (SYSTICK_INTERVAL_MS * 1000U)) != 0) {
		printf("SysTick init failed!");
		NVIC_SystemReset();
	}
}

void SysTick_Handler(void)
{
	sysTick++;
}

uint32_t SysTick_GetTicks(void)
{
	return sysTick;
}
