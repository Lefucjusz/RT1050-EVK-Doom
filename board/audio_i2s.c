/*
 * audio_i2s.c
 *
 *  Created on: Aug 14, 2024
 *      Author: lefucjusz
 */

#include "audio_i2s.h"
#include "fsl_codec_common.h"
#include "fsl_wm8960.h"
#include "fsl_edma.h"
#include "fsl_dmamux.h"
#include "fsl_sai.h"
#include "board.h"
#include "MIMXRT1052.h"
#include <stdio.h>
#include <math.h>

#define AUDIO_DMAMUX 		DMAMUX
#define AUDIO_DMA 			DMA0
#define AUDIO_EDMA_CHANNEL 	0U
#define AUDIO_SAI_TX_SOURCE kDmaRequestMuxSai1Tx

#define AUDIO_SAI 				SAI1
#define AUDIO_SAI_CHANNEL 		0
#define AUDIO_SAI_BIT_WIDTH 	kSAI_WordWidth16bits
#define AUDIO_SAI_CHANNELS 		kSAI_Stereo
#define AUDIO_SAI_SAMPLE_RATE 	kSAI_SampleRate11025Hz
#define AUDIO_SAI_TX_SYNC_MODE 	kSAI_ModeAsync
#define AUDIO_SAI_MASTER_SLAVE 	kSAI_Master

#define AUDIO_MCLK_FREQ_WM8960 		4233600U // MCLK = 384 * Fs
#define AUDIO_BIT_WIDTH_WM8960 		kWM8960_AudioBitWidth16bit
#define AUDIO_SAMPLE_RATE_WM8960 	kWM8960_AudioSampleRate11025Hz

typedef enum
{
	BUFFER_EMPTY = 0,
	BUFFER_FILLED,
	BUFFER_PLAYING
} audio_buffer_state_t;

static volatile audio_buffer_state_t audioBufferStates[AUDIO_BUFFERS];

AT_NONCACHEABLE_SECTION_ALIGN(static int16_t audioBuffer[AUDIO_BUFFER_TOTAL_SIZE_SAMPLES], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static edma_tcd_t edmaTcd, 32);

static edma_handle_t dmaHandle;
static edma_config_t dmaConfig;
static edma_transfer_config_t transferConfig;
static sai_transceiver_t saiConfig;
static codec_handle_t codecHandle;

static wm8960_config_t wm8960Config = {
    .i2cConfig = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = BOARD_CODEC_I2C_CLOCK_FREQ},
    .route     = kWM8960_RoutePlayback,
    .leftInputSource  = kWM8960_InputClosed,
    .rightInputSource = kWM8960_InputClosed,
    .playSource       = kWM8960_PlaySourceDAC,
    .slaveAddress     = WM8960_I2C_ADDR,
    .bus              = kWM8960_BusI2S,
    .format = {.mclk_HZ = AUDIO_MCLK_FREQ_WM8960, .sampleRate = AUDIO_SAMPLE_RATE_WM8960, .bitWidth = AUDIO_BIT_WIDTH_WM8960},
    .master_slave = false,
};

static codec_config_t codecConfig = {.codecDevType = kCODEC_WM8960, .codecDevConfig = &wm8960Config};

static void BOARD_EnableSaiMclkOutput(bool enable)
{
	if (enable) {
		IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK;
	}
	else {
		IOMUXC_GPR->GPR1 &= ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR_MASK;
	}
}

static void EDMA_Tx_Callback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
	if (transferDone) {
		memset(&audioBuffer[AUDIO_BUFFER_SIZE_SAMPLES], 0, AUDIO_BUFFER_SIZE_BYTES); // Avoid glitching if buffer not filled in time
		audioBufferStates[1] = BUFFER_EMPTY;
		audioBufferStates[0] = BUFFER_PLAYING;
	}
	else {
		memset(&audioBuffer[0], 0, AUDIO_BUFFER_SIZE_BYTES); // Avoid glitching if buffer not filled in time
		audioBufferStates[0] = BUFFER_EMPTY;
		audioBufferStates[1] = BUFFER_PLAYING;
	}
}

void audio_i2s_init(void)
{
	/* Enable MCLK clock */
	BOARD_EnableSaiMclkOutput(true);

	/* Init DMAMUX */
	DMAMUX_Init(AUDIO_DMAMUX);
	DMAMUX_SetSource(AUDIO_DMAMUX, AUDIO_EDMA_CHANNEL, AUDIO_SAI_TX_SOURCE);
	DMAMUX_EnableChannel(AUDIO_DMAMUX, AUDIO_EDMA_CHANNEL);

	/* Create EDMA handle */
	EDMA_GetDefaultConfig(&dmaConfig);
	EDMA_Init(AUDIO_DMA, &dmaConfig);
	EDMA_CreateHandle(&dmaHandle, AUDIO_DMA, AUDIO_EDMA_CHANNEL);
	EDMA_SetCallback(&dmaHandle, EDMA_Tx_Callback, NULL);
	EDMA_ResetChannel(AUDIO_DMA, dmaHandle.channel);

	/* Initialize SAI */
	SAI_Init(AUDIO_SAI);

	/* Configure I2S */
	const uint32_t audioPllTotalDiv = (CLOCK_GetDiv(kCLOCK_Sai1PreDiv) + 1) * (CLOCK_GetDiv(kCLOCK_Sai1Div) + 1);
	const uint32_t saiSourceClockHz = CLOCK_GetFreq(kCLOCK_AudioPllClk) / audioPllTotalDiv;
	SAI_GetClassicI2SConfig(&saiConfig, AUDIO_SAI_BIT_WIDTH, AUDIO_SAI_CHANNELS, 1U << AUDIO_SAI_CHANNEL);
	saiConfig.syncMode = AUDIO_SAI_TX_SYNC_MODE;
	saiConfig.masterSlave = AUDIO_SAI_MASTER_SLAVE;
	SAI_TxSetConfig(AUDIO_SAI, &saiConfig);
	SAI_TxSetBitClockRate(AUDIO_SAI, saiSourceClockHz, AUDIO_SAI_SAMPLE_RATE, AUDIO_SAI_BIT_WIDTH, AUDIO_CHANNELS);

	/* Initialize codec */
	if (CODEC_Init(&codecHandle, &codecConfig) != kStatus_Success) {
		printf("Failed to initialize WM8960 codec!\n");
		return;
	}

	/* Prepare EDMA transfer */
	const uintptr_t saiDestAddress = SAI_TxGetDataRegisterAddress(AUDIO_SAI, AUDIO_SAI_CHANNEL);
	EDMA_PrepareTransfer(&transferConfig,			// Transfer config
						 audioBuffer, 				// Source
						 AUDIO_SAMPLE_SIZE_BYTES,   // Source width
						 (void *)saiDestAddress, 	// Destination
						 AUDIO_SAMPLE_SIZE_BYTES,	// Destination width
						 (FSL_FEATURE_SAI_FIFO_COUNT - saiConfig.fifo.fifoWatermark) * AUDIO_SAMPLE_SIZE_BYTES, // No idea what this is
						 AUDIO_BUFFER_TOTAL_SIZE_BYTES,	// Total transfer size
						 kEDMA_MemoryToPeripheral);	// Transfer type

	/* Prepare TCD */
	EDMA_TcdSetTransferConfig(&edmaTcd, &transferConfig, &edmaTcd);
	EDMA_TcdEnableInterrupts(&edmaTcd, kEDMA_MajorInterruptEnable | kEDMA_HalfInterruptEnable);
	EDMA_InstallTCD(AUDIO_DMA, dmaHandle.channel, &edmaTcd);

	/* Start EDMA transfer */
	EDMA_StartTransfer(&dmaHandle);

	/* Enable DMA request */
	SAI_TxEnableDMA(AUDIO_SAI, kSAI_FIFORequestDMAEnable, true);

	/* Enable SAI Tx */
	SAI_TxEnable(AUDIO_SAI, true);

	/* Enable channel FIFO */
	SAI_TxSetChannelFIFOMask(AUDIO_SAI, 1U << AUDIO_SAI_CHANNEL);
}

void audio_i2s_deinit(void)
{
	EDMA_AbortTransfer(&dmaHandle);
	SAI_Deinit(AUDIO_SAI);
	if (CODEC_Deinit(&codecHandle) != kStatus_Success) {
		printf("Failed to deinitialize WM8960 codec!\n");
	}
	EDMA_Deinit(AUDIO_DMA);
	DMAMUX_Deinit(AUDIO_DMAMUX);
}

int16_t *audio_i2s_get_buffer(void)
{
	for (size_t i = 0; i < AUDIO_BUFFERS; ++i) {
		if (audioBufferStates[i] == BUFFER_EMPTY) {
			audioBufferStates[i] = BUFFER_FILLED;
			return &audioBuffer[i * AUDIO_BUFFER_SIZE_SAMPLES];
		}
	}
	return NULL;
}
