/*
 * i_sound_rt1051.c
 *
 *  Created on: Aug 14, 2024
 *      Author: lefucjusz
 */

#include "i_sound.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"
#include "deh_str.h"
#include "audio_i2s.h"
#include <string.h>
#include <stdio.h>

#define LUMP_HEADER_LENGTH 8
#define NUM_CHANNELS 16
#define BYTE_SAMPLE_OFFSET 128 //  To convert unsigned to signed

typedef struct
{
	const uint8_t *data;
	size_t position;
	size_t length;
	uint8_t volume_l;
	uint8_t volume_r;
} channel_data_t;

static channel_data_t channels[NUM_CHANNELS];
static boolean is_sfx_prefix;

static boolean I_RT1051_CacheSfx(sfxinfo_t *sfx, int handle)
{
	/* Load the sound to cache */
	const uint8_t *lumpdata = W_CacheLumpNum(sfx->lumpnum, PU_STATIC); // TODO this is never freed?
	const size_t lumplen = W_LumpLength(sfx->lumpnum);

	/* Validate */
	if (lumplen < LUMP_HEADER_LENGTH || lumpdata[0] != 0x03 || lumpdata[1] != 0x00) { // TODO magic numbers
		W_ReleaseLumpNum(sfx->lumpnum);
		return false;
	}

	/* Get sample rate and length */
	const int samplerate = (lumpdata[3] << 8) | (lumpdata[2] << 0);
	size_t length = (lumpdata[7] << 24) | (lumpdata[6] << 16) | (lumpdata[5] << 8) | (lumpdata[4] << 0);

	/* Validate:
	 * - sound length cannot be greater than the lump itself;
	 * - sound lumps shorter than 49 samples are discarded, as this is how DMX
	 * 	 library worked, although the cutoff length seems to vary depending on
	 * 	 the sample rate. This behavior is not fully understood. */
	if (length > (lumplen - LUMP_HEADER_LENGTH) || length <= 48) {
		W_ReleaseLumpNum(sfx->lumpnum);
		return false;
	}

	/* DMX sound library seems to skip first and last 16 bytes of the
	 * lump - the reason is unknown. */
	lumpdata += 16;
	length -= 32;

	/* Setup channel */
//	printf("%s: channel: %d, samplerate: %d, length: %d\n", __FUNCTION__, handle, samplerate, length);

	channels[handle].data = lumpdata;
	channels[handle].position = 0;
	channels[handle].length = length;

	return true;
}

static void I_RT1051_ClearSoundOnChannel(int channel)
{
	memset(&channels[channel], 0, sizeof(channel_data_t));
}

static void I_RT1051_MixChannels(int16_t *sample_l, int16_t *sample_r)
{
	/* Clear samples */
	*sample_l = 0;
	*sample_r = 0;

	/* Mix all channels */
	for (size_t i = 0; i < NUM_CHANNELS; ++i) {
		/* Skip inactive */
		channel_data_t *ch = &channels[i];
		if (ch->data == NULL) {
			continue;
		}

		/* Get next sample, convert from unsigned to signed */
		const int16_t sample = ch->data[ch->position] - BYTE_SAMPLE_OFFSET;
		ch->position++;

		/* Entire sound has been played back */
		if (ch->position >= ch->length) {
			I_RT1051_ClearSoundOnChannel(i);
		}

		/* Scale by volume and accumulate */
		*sample_l += sample * ch->volume_l;
		*sample_r += sample * ch->volume_r;
	}
}

static boolean I_RT1051_InitSound(boolean use_sfx_prefix)
{
	is_sfx_prefix = use_sfx_prefix;

	for (size_t i = 0; i < NUM_CHANNELS; ++i) {
		I_RT1051_ClearSoundOnChannel(i);
	}

	audio_i2s_init();

	return true;
}

static void I_RT1051_ShutdownSound(void)
{
	audio_i2s_deinit();
}

static int I_RT1051_GetSfxLumpNum(sfxinfo_t *sfxinfo)
{
	const sfxinfo_t *sfx = sfxinfo;
	char namebuf[16];

	/* Linked sfx lumps? Get the lump number for the sound linked to. */
	if (sfx->link != NULL) {
		sfx = sfx->link;
	}

	/* Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't do this. */
	if (is_sfx_prefix) {
		M_snprintf(namebuf, sizeof(namebuf), "ds%s", DEH_String(sfx->name));
	}
	else {
		M_StringCopy(namebuf, DEH_String(sfx->name), sizeof(namebuf));
	}

	return W_GetNumForName(namebuf);
}

static void I_RT1051_UpdateSound(void)
{
	int16_t *buffer = audio_i2s_get_buffer();
	if (buffer == NULL) {
		return;
	}

	for (size_t i = 0; i < AUDIO_BUFFER_SIZE_FRAMES; ++i) {
		int16_t sample_l, sample_r;
		I_RT1051_MixChannels(&sample_l, &sample_r);

		/* Push to buffer */
		buffer[2 * i] = sample_l;
		buffer[2 * i + 1] = sample_r;
	}
}

static void I_RT1051_UpdateSoundParams(int channel, int vol, int sep)
{
	/* Validate channels range */
	if (channel < 0 || channel >= NUM_CHANNELS) {
		return;
	}

	int left = ((254 - sep) * vol) / 127;
	int right = ((sep) * vol) / 127;

	if (left < 0) {
		left = 0;
	}
	else if (left > 255) {
		left = 255;
	}

	if (right < 0) {
		right = 0;
	}
	else if (right > 255) {
		right = 255;
	}

	channels[channel].volume_l = left;
	channels[channel].volume_r = right;
}

static int I_RT1051_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep)
{
	/* Validate channels range */
	if (channel < 0 || channel >= NUM_CHANNELS) {
		return -1;
	}

	/* Clear sound already playing on the given channel */
	I_RT1051_ClearSoundOnChannel(channel);

	/* Get sound data */
	if (!I_RT1051_CacheSfx(sfxinfo, channel)) {
		printf("Error caching SFX!\n");
		return -1;
	}

	/* Update parameters */
	I_RT1051_UpdateSoundParams(channel, vol, sep);

	return channel;
}

static void I_RT1051_StopSound(int channel)
{
	/* Validate channels range */
	if (channel < 0 || channel >= NUM_CHANNELS) {
		return;
	}

	I_RT1051_ClearSoundOnChannel(channel);
}

static boolean I_RT1051_SoundIsPlaying(int channel)
{
	/* Validate channels range */
	if (channel < 0 || channel >= NUM_CHANNELS) {
		return false;
	}

	return (channels[channel].data != NULL);
}

static void I_RT1051_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
	// No-op
}

static snddevice_t sound_sdl_devices[] =
{
    SNDDEVICE_SB,
    SNDDEVICE_PAS,
    SNDDEVICE_GUS,
    SNDDEVICE_WAVEBLASTER,
    SNDDEVICE_SOUNDCANVAS,
    SNDDEVICE_AWE32
};

sound_module_t DG_sound_module =
{
    sound_sdl_devices,
    arrlen(sound_sdl_devices),
	I_RT1051_InitSound,
	I_RT1051_ShutdownSound,
	I_RT1051_GetSfxLumpNum,
	I_RT1051_UpdateSound,
	I_RT1051_UpdateSoundParams,
	I_RT1051_StartSound,
	I_RT1051_StopSound,
	I_RT1051_SoundIsPlaying,
	I_RT1051_PrecacheSounds
};
