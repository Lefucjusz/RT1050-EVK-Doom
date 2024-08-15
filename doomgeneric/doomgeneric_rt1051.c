/*
 * doomgeneric_rt1051.c
 *
 *  Created on: Oct 26, 2023
 *      Author: lefucjusz
 */

#include <math.h>
#include <stdio.h>
#include "fsl_common.h"
#include "doomgeneric.h"
#include "doomkeys.h"
#include "display.h"
#include "key_buffer.h"
#include "systick.h"

static inline uint8_t map_key(uint8_t inkey)
{
	switch (inkey) {
	case 28:
		return 'y';
	case 30:
		return '1';
	case 31:
		return '2';
	case 32:
		return '3';
	case 33:
		return '4';
	case 34:
		return '5';
	case 35:
		return '6';
	case 36:
		return '7';
	case 37:
		return '8';
	case 38:
		return '9';
	case 39:
		return '0';
	case 40:
		return KEY_ENTER;
	case 41:
		return KEY_ESCAPE;
	case 42:
		return KEY_BACKSPACE;
	case 43:
		return KEY_TAB;
	case 44:
		return KEY_USE;
	case 79:
		return KEY_RIGHTARROW;
	case 80:
		return KEY_LEFTARROW;
	case 81:
		return KEY_DOWNARROW;
	case 82:
		return KEY_UPARROW;
	case 253:
		return KEY_FIRE;
	case 254:
		return KEY_RSHIFT;
	default:
		return 0;
	}
}

void DG_Init(void)
{
	display_init(DG_ScreenBuffer);
}

void DG_DrawFrame()
{
	display_refresh(DG_ScreenBuffer);
}

void DG_SleepMs(uint32_t ms)
{
	const uint32_t start_tick = SysTick_GetTicks();
	while ((SysTick_GetTicks() - start_tick) < ms) {
		__asm__ __volatile__("wfi\n");
	}
}

uint32_t DG_GetTicksMs(void)
{
	return SysTick_GetTicks();
}

int DG_GetKey(int *pressed, unsigned char *key)
{
	const key_event_t *event = key_buffer_get_event();
	if (event != NULL) {
		*key = map_key(event->key);
		*pressed = event->state;
		return 1;
	}
	return 0;
}

void DG_SetWindowTitle(const char *title)
{

}
