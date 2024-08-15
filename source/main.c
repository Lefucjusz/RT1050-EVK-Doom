/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_sd.h"
#include "ff.h"
#include "fsl_sd_disk.h"
#include "sdmmc_config.h"
#include "peripherals.h"
#include "doomgeneric.h"
#include "systick.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS fatfs;
static const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static status_t sdcardWaitCardInsert(void)
{
    BOARD_SD_Config(&g_sd, NULL, BOARD_SDMMC_SD_HOST_IRQ_PRIORITY, NULL);

    /* SD host init function */
    if (SD_HostInit(&g_sd) != kStatus_Success) {
        printf("SD host init fail\n");
        return kStatus_Fail;
    }

    /* wait card insert */
    if (SD_PollingCardInsert(&g_sd, kSD_Inserted) == kStatus_Success) {
    	printf("SD card present\n");
        /* power off card */
        SD_SetCardPower(&g_sd, false);
        /* power on the card */
        SD_SetCardPower(&g_sd, true);
    }
    else {
    	printf("Card detection failed\n");
        return kStatus_Fail;
    }

    return kStatus_Success;
}

static void cleanup(void)
{
	doomgeneric_Destroy();
	f_mount(NULL, driverNumberBuffer, 0);
}

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{
    /* Init board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_InitUSDHCPins();
    BOARD_InitI2C1Pins();
    BOARD_InitSAIPins();
    BOARD_InitSemcPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitBootPeripherals();

    SysTick_Init();

    if (sdcardWaitCardInsert() != kStatus_Success) {
    	printf("Failed to init SD card\n");
    	return -1;
	}

    memset(&fatfs, 0, sizeof(fatfs));
	if (f_mount(&fatfs, driverNumberBuffer, 1)) {
	   printf("Failed to mount volume\n");
	   return -1;
	}

	/* Register atexit function to clean up environment */
	atexit(cleanup);

	/* Main loop */
    doomgeneric_Create(0, NULL);
    while (1) {
    	USB_HostTasks();
    	doomgeneric_Tick();
    }

    return 0;
}

