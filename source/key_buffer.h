/*
 * key_buffer.h
 *
 *  Created on: Aug 13, 2024
 *      Author: lefucjusz
 */

#ifndef KEY_BUFFER_H_
#define KEY_BUFFER_H_

#include <stdint.h>
#include <stddef.h>

#define KEY_BUFFER_SIZE 8

typedef enum {
	KEY_RELEASED = 0,
	KEY_PRESSED = 1
} key_state_t;

typedef struct {
	uint8_t key;
	key_state_t state;
} key_event_t;

typedef struct {
	size_t write_index;
	size_t read_index;
	key_event_t event[KEY_BUFFER_SIZE];
} key_buffer_t;


void key_buffer_init(void);

void key_buffer_push_event(uint8_t key, key_state_t state);
const key_event_t *key_buffer_get_event(void);

#endif /* KEY_BUFFER_H_ */
