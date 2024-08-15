/*
 * key_buffer.c
 *
 *  Created on: Aug 13, 2024
 *      Author: lefucjusz
 */

#include "key_buffer.h"

static key_buffer_t key_buffer;

void key_buffer_init(void)
{
	key_buffer.read_index = 0;
	key_buffer.write_index = 0;
}

void key_buffer_push_event(uint8_t key, key_state_t state)
{
	key_buffer.event[key_buffer.write_index].key = key;
	key_buffer.event[key_buffer.write_index].state = state;
	key_buffer.write_index++;
	key_buffer.write_index %= KEY_BUFFER_SIZE;
}

const key_event_t *key_buffer_get_event(void)
{
	/* Key buffer empty */
	if (key_buffer.read_index == key_buffer.write_index) {
		return NULL;
	}

	const key_event_t *event = &key_buffer.event[key_buffer.read_index];
	key_buffer.read_index++;
	key_buffer.read_index %= KEY_BUFFER_SIZE;
	return event;
}
