/*
 * display.c
 *
 *  Created on: Oct 27, 2023
 *      Author: lefucjusz
 */

#include "display.h"
#include <fsl_clock.h>
#include <fsl_elcdif.h>
#include <fsl_gpio.h>
#include <fsl_cache.h>
#include <stdbool.h>

static volatile bool frame_pending = false;

static void display_clock_init(void)
{
    /*
     * The desired output frame rate is 60Hz. So the pixel clock frequency is:
     * (480 + 41 + 4 + 18) * (272 + 10 + 4 + 2) * 60 = 9.2M.
     * Here set the LCDIF pixel clock to 9.3M.
     */

    /*
     * Initialize the Video PLL.
     * Video PLL output clock is OSC24M * (loopDivider + (denominator / numerator)) / postDivider = 93MHz.
     */
    const clock_video_pll_config_t config = {
        .loopDivider = 31,
        .postDivider = 8,
        .numerator   = 0,
        .denominator = 0,
    };
    CLOCK_InitVideoPll(&config);

    /*
     * 000 derive clock from PLL2
     * 001 derive clock from PLL3 PFD3
     * 010 derive clock from PLL5
     * 011 derive clock from PLL2 PFD0
     * 100 derive clock from PLL2 PFD1
     * 101 derive clock from PLL3 PFD1
     */
    CLOCK_SetMux(kCLOCK_LcdifPreMux, 2);
    CLOCK_SetDiv(kCLOCK_LcdifPreDiv, 4);
    CLOCK_SetDiv(kCLOCK_LcdifDiv, 1);
}

static void display_backlight_init(void)
{
    const gpio_pin_config_t config = {
        kGPIO_DigitalOutput,
        1,
        kGPIO_NoIntmode,
    };

    GPIO_PinInit(DISPLAY_BACKLIGHT_GPIO, DISPLAY_BACKLIGHT_PIN, &config);
}

void display_init(const uint16_t *buffer)
{
    /* Initialize the display. */
    const elcdif_rgb_mode_config_t config = {
        .panelWidth    = DISPLAY_WIDTH,
        .panelHeight   = DISPLAY_HEIGHT,
        .hsw           = DISPLAY_HSW,
        .hfp           = DISPLAY_HFP,
        .hbp           = DISPLAY_HBP,
        .vsw           = DISPLAY_VSW,
        .vfp           = DISPLAY_VFP,
        .vbp           = DISPLAY_VBP,
        .polarityFlags = DISPLAY_POL_FLAGS,
        .bufferAddr  = (uint32_t)buffer,
        .pixelFormat = kELCDIF_PixelFormatRGB565,
        .dataBus     = DISPLAY_LCDIF_DATA_BUS,
    };

    frame_pending = false;

    display_clock_init();

    ELCDIF_RgbModeInit(LCDIF, &config);

    ELCDIF_EnableInterrupts(LCDIF, kELCDIF_CurFrameDoneInterruptEnable);
    NVIC_EnableIRQ(LCDIF_IRQn);
    ELCDIF_RgbModeStart(LCDIF);

    display_backlight_init();
}

void display_refresh(const uint16_t *buffer)
{
	DCACHE_CleanInvalidateByRange((uint32_t)buffer, DISPLAY_BUFFER_SIZE);
	ELCDIF_SetNextBufferAddr(LCDIF, (uint32_t)buffer);

	frame_pending = true;

	while (frame_pending) {}
}

void LCDIF_IRQHandler(void)
{
    const uint32_t status = ELCDIF_GetInterruptStatus(LCDIF);
    ELCDIF_ClearInterruptStatus(LCDIF, status);

    if (frame_pending && (status & kELCDIF_CurFrameDone)) {
    	frame_pending = false;
    }

    __DSB();
}
