/*
 * DISPLAY_.h
 *
 *  Created on: Oct 27, 2023
 *      Author: lefucjusz
 */

#pragma once

#include <stdint.h>

#define DISPLAY_BACKLIGHT_GPIO GPIO2
#define DISPLAY_BACKLIGHT_PIN 31

#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 272

#define DISPLAY_HSW 41
#define DISPLAY_HFP 4
#define DISPLAY_HBP 8
#define DISPLAY_VSW 10
#define DISPLAY_VFP 4
#define DISPLAY_VBP 2

#define DISPLAY_POL_FLAGS (kELCDIF_DataEnableActiveHigh | kELCDIF_VsyncActiveLow | kELCDIF_HsyncActiveLow | kELCDIF_DriveDataOnRisingClkEdge)
#define DISPLAY_LCDIF_DATA_BUS kELCDIF_DataBus16Bit

#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t))

void display_init(const uint16_t *buffer);
void display_refresh(const uint16_t *buffer);
