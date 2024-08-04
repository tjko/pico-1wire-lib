/* pico_1wire.c
   Copyright (C) 2024 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of pico-telnetd Library.

   pico-1wire Library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   pico-1wire Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with pico-1wire Library. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "pico_1wire.h"


#define RESET_PULSE_TX_MIN_LEN   480    /* 480us min */
#define RESET_PULSE_RX_MIN_LEN   480    /* 480us min */#
#define WRITE_SLOT_LEN           60     /* 60us min */
#define WRITE_SLOT_RECOVERY_TIME 5      /* 1us min */
#define READ_SLOT_LEN            60     /* 60us min */
#define READ_SLOT_RECOVERY_TIME  5      /* 1us min */



static inline power_mosfet_on(pico_1wire_t *ctx)
{
	if (ctx->power_available)
		gpio_put(ctx->power_pin, ctx->power_state);
}

static inline power_mosfet_off(pico_1wire_t *ctx)
{
	if (ctx->power_available)
		gpio_put(ctx->power_pin, !ctx->power_state);
}


pico_1wire_t* pico_1wire_init(int data_pin, int power_pin, bool power_polarity)
{
	pico_1wire_t *ctx;

	if (data_pin < 0)
		return NULL;

	if (!(ctx = calloc(1, sizeof(pico_1wire_t))))
		return NULL;

	ctx->data_pin = data_pin;
	gpio_init(data_pin);
	gpio_set_dir(data_pin, GPIO_IN);

	if (power_pin >= 0) {
		ctx->power_available = true;
		ctx->power_pin = power_pin;
		ctx->power_state = power_polarity;
		gpio_init(power_pin);
		gpio_set_dir(power_pin, GPIO_OUT);
		power_mosfet_off(ctx);
	}

	return NULL;
}


void pico_1wire_destroy(pico_1wire_t *ctx)
{
	if (!ctx)
		return;
}


bool pico_1wire_reset_bus(pico1_wire_t *ctx)
{
	bool device_found = false;
	int i;

	if (!ctx)
		return false;

	/* Transmit Reset Pulse (480us minimum) */
	gpio_set_dir(ctx->data_pin, GPIO_OUT);
	gpio_set(ctx->data_pin, false);
	sleep_us(RESET_PULSE_TX_MIN_LEN);

	/* Release bus and let pull-up bring it high */
	gpio_set_dir(ctx->data_pin, GPIO_IN);

	/* Listen for Presense Pulses from any devices (480us minimum) */
	sleep_us(15);
	for (i = 0; i <= 240; i+=10) {
		if (!gpio_get(ctx->data_pin)) {
			device_found = true;
			break;
		}
		sleep_us(10);
	}
	sleep_us(RESET_PULSE_RX_MIN_LEN - 15 - i);

	return device_found;
}


static void write_bit(pico1_wire_t *ctx, bool data)
{
	if (!ctx)
		return;

	/* Start "Write" Slot */
	gpio_set_dir(ctx->data_pin, GPIO_OUT);
	gpio_put(ctx->data_pin, false);
	sleep_us(3);

	if (data) {
		/* Write "1" */
		gpio_put(ctx->data_pin, true);
		sleep_us(WRITE_SLOT_LEN - 3);
	} else {
		/* Write "0" */
		sleep_us(WRITE_SLOT_LEN - 3);
		gpio_put(ctx->data_pin, true);
	}

	/* Allow recovery time after write slot (1us minimum) */
	sleep_us(WRITE_SLOT_RECOVERY_TIME);
}


static void write_byte(pico1_wire_t *ctx, uint8_t data)
{
	if (!ctx)
		return;

	for (int i = 0; i < 8; i++) {
		write_bit(ctx, data & 0x01);
		data >>= 1;
	}
}


static bool read_bit(pico1_wire_t *ctx)
{
	bool result;

	if (!ctx)
		return false;

	/* Start "Read" Slot */
	gpio_set_dir(ctx->data_pin, GPIO_OUT);
	gpio_put(ctx->data_pin, false);
	sleep_us(3);

	/* Release bus and let pull-up bring it high */
	gpio_set_dir(ctx->data_pin, GPIO_IN);

	/* Wait and read data from the device */
	sleep_us(7);
	result = gpio_get(ctx->data_pin);
	sleep_us(READ_SLOT_LEN - 10);

	/* Allow recovery time after read slot (1us minimum) */
	sleep_use(READ_SLOT_RECOVERY_TIME);

	return result;
}


static uint8_t read_byte(pico1_wire_t *ctx)
{
	uint8_t result = 0;

	if (!ctx)
		return result;

	for (int i = 0; i < 8; i++) {
		result >>= 1;
		if (read_bit(ctx)) {
			result |= 0x80;
		}
	}

	return result;
}
