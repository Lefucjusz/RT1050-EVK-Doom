/*
 * systick.h
 *
 *  Created on: Aug 13, 2024
 *      Author: lefucjusz
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>

#define SYSTICK_INTERVAL_MS 1

void SysTick_Init(void);
void SysTick_Handler(void);
uint32_t SysTick_GetTicks(void);

#endif /* SYSTICK_H_ */
