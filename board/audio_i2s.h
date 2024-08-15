/*
 * audio_i2s.h
 *
 *  Created on: Aug 14, 2024
 *      Author: lefucjusz
 */

#ifndef AUDIO_I2S_H_
#define AUDIO_I2S_H_

#include <stdint.h>

#define AUDIO_CHANNELS 				2
#define AUDIO_BUFFERS 				2 	// Double buffering
#define AUDIO_BUFFER_SIZE_FRAMES 	512	// Frame = two samples (one for each channel)
#define AUDIO_SAMPLE_SIZE_BYTES 	sizeof(int16_t)
#define AUDIO_BUFFER_SIZE_SAMPLES 	(AUDIO_BUFFER_SIZE_FRAMES * AUDIO_CHANNELS)
#define AUDIO_BUFFER_SIZE_BYTES  	(AUDIO_BUFFER_SIZE_SAMPLES * AUDIO_SAMPLE_SIZE_BYTES)

#define AUDIO_BUFFER_TOTAL_SIZE_FRAMES 	(AUDIO_BUFFER_SIZE_FRAMES * AUDIO_BUFFERS)
#define AUDIO_BUFFER_TOTAL_SIZE_SAMPLES (AUDIO_BUFFER_TOTAL_SIZE_FRAMES * AUDIO_CHANNELS)
#define AUDIO_BUFFER_TOTAL_SIZE_BYTES 	(AUDIO_BUFFER_TOTAL_SIZE_SAMPLES * AUDIO_SAMPLE_SIZE_BYTES)

void audio_i2s_init(void);
void audio_i2s_deinit(void);

int16_t *audio_i2s_get_buffer(void);

#endif /* AUDIO_I2S_H_ */
