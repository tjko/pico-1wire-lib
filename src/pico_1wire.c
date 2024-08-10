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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "pico_1wire.h"


/* ROM Commands */
#define CMD_SEARCH         0xF0
#define CMD_READ           0x33
#define CMD_MATCH          0x55
#define CMD_SKIP           0xCC
#define CMD_ALARM_SEARCH   0xEC

/* Function Commands */
#define CMD_CONVERT            0x44
#define CMD_WRITE_SCRATCHPAD   0x4E
#define CMD_READ_SCRATCHPAD    0xBE
#define CMD_COPY_SCRATCHPAD    0x48
#define CMD_RECALL             0xB8
#define CMD_READ_POWER_SUPPLY  0xB4

/* 1-Wire Device Family Codes */
#define FAMILY_CODE_DS18S20      0x10  /* Temperature (9bit) */
#define FAMILY_CODE_DS1822       0x22  /* Temperature (9-12bit) */
#define FAMILY_CODE_DS18B20      0x28  /* Temperature (9-12bit) */
#define FAMILY_CODE_MAX31820     0x28  /* Temperature (9-12bit) */
#define FAMILY_CODE_DS1825       0x3B  /* Temperature (9-12bit) */
#define FAMILY_CODE_MAX31826     0x3B  /* Temperature (12bit) + 1k EEPROM */
#define FAMILY_CODE_DS28EA00     0x42  /* Temperature (9-12bit) + IO */

/* Timings */
#define RESET_PULSE_TX_MIN_LEN   480    /* 480us min */
#define RESET_PULSE_RX_MIN_LEN   480    /* 480us min */
#define WRITE_SLOT_LEN           60     /* 60us min */
#define WRITE_SLOT_RECOVERY_TIME 5      /* 1us min */
#define READ_SLOT_LEN            60     /* 60us min */
#define READ_SLOT_RECOVERY_TIME  5      /* 1us min */

#define MAX_TEMP_CONVERSION_TIME 750    /* 750ms */


#define ADDR_FAMILY_CODE(x) ((uint64_t)(x) >> 56)
#define NULL_BUS_ADDRESS  (uint64_t)0



static const uint8_t pico_1wire_crc8_lookup_table[] = {
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};


static inline uint8_t crc8(uint8_t crc, uint8_t data)
{
	return pico_1wire_crc8_lookup_table[crc ^ data];
}


static inline void uint64_set_bit(uint64_t *var, uint bit, bool value)
{
	*var = (*var & ~((uint64_t)1 << bit)) | ((uint64_t)value << bit);
}


static inline void power_mosfet_on(pico_1wire_t *ctx)
{
	if (ctx->power_available)
		gpio_put(ctx->power_pin, ctx->power_state);
}


static inline void power_mosfet_off(pico_1wire_t *ctx)
{
	if (ctx->power_available)
		gpio_put(ctx->power_pin, !ctx->power_state);
}


static void write_bit(pico_1wire_t *ctx, bool data)
{
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


static void write_byte(pico_1wire_t *ctx, uint8_t data)
{
	for (int i = 0; i < 8; i++) {
		write_bit(ctx, data & 0x01);
		data >>= 1;
	}
}


static bool read_bit(pico_1wire_t *ctx)
{
	/* Start "Read" Slot */
	gpio_set_dir(ctx->data_pin, GPIO_OUT);
	gpio_put(ctx->data_pin, false);
	sleep_us(3);

	/* Release bus and let pull-up bring it high */
	gpio_set_dir(ctx->data_pin, GPIO_IN);

	/* Wait and read data from the device */
	sleep_us(7);
	bool result = gpio_get(ctx->data_pin);
	sleep_us(READ_SLOT_LEN - 10);

	/* Allow recovery time after read slot (1us minimum) */
	sleep_us(READ_SLOT_RECOVERY_TIME);

	return result;
}


static uint8_t read_byte(pico_1wire_t *ctx)
{
	uint8_t result = 0;

	for (int i = 0; i < 8; i++) {
		result >>= 1;
		if (read_bit(ctx)) {
			result |= 0x80;
		}
	}

	return result;
}


static bool find_next_device(pico_1wire_t *ctx, uint64_t *addr, bool *done, uint *last_discrepancy)
{
	bool result = false;
	uint rom_bit_index = 1;
	uint discrepancy = 0;
	bool bit_a, bit_b;

	/* This implementation is based on one described in the DS18B20 datasheet. */

	if (*done) {
		*done = false;
		return result;
	}

	if (!pico_1wire_reset_bus(ctx)) {
		*last_discrepancy = 0;
		return result;
	}

	/* Send Search ROM command */
	write_byte(ctx, CMD_SEARCH);

	do {
		/* Read Responses */
		bit_a = read_bit(ctx);
		bit_b = read_bit(ctx);

		if (bit_a & bit_b) { /* Both bits 1 */
			*last_discrepancy = 0;
			return result;
		}
		if (bit_a == bit_b) { /* Both bits 0 */
			if (rom_bit_index == *last_discrepancy) {
				uint64_set_bit(addr, rom_bit_index - 1, true);
			}
			else if (rom_bit_index > *last_discrepancy) {
				uint64_set_bit(addr, rom_bit_index - 1, false);
				discrepancy = rom_bit_index;
			}
			else if (!( *addr & ((uint64_t)1 << (rom_bit_index - 1)))) {
				discrepancy = rom_bit_index;
			}
		} else {
			uint64_set_bit(addr, rom_bit_index - 1, bit_a);
		}
		write_bit(ctx, (*addr & ((uint64_t)1 << (rom_bit_index - 1 ))));
		rom_bit_index++;
	} while (rom_bit_index <= 64);

	*last_discrepancy = discrepancy;
	if (*last_discrepancy == 0)
		*done = true;
	result = true;

	return result;
}


static int match_rom(pico_1wire_t *ctx, uint64_t addr)
{
	if (!pico_1wire_reset_bus(ctx))
		return 1;

	if (addr ==  0) {
		/* Send Skip ROM command */
		write_byte(ctx, CMD_SKIP);
	} else {
		/* Send Match ROM command */
		write_byte(ctx, CMD_MATCH);
		for (int i = 0; i < 8; i++) {
			write_byte(ctx, ((uint8_t *)&addr)[7 - i]);
		}
	}

	return 0;
}



/*****************************/
/* Exposed Library Functions */


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

	ctx->psu_present = true;

	/* Check if any device in the bus uses phantom power. */
	pico_1wire_read_power_supply(ctx, 0, NULL);

	return ctx;
}


void pico_1wire_destroy(pico_1wire_t *ctx)
{
	if (!ctx)
		return;

	gpio_set_dir(ctx->data_pin, GPIO_IN);

	if (ctx->power_available) {
		gpio_set_dir(ctx->data_pin, GPIO_IN);
	}

	free(ctx);
}


bool pico_1wire_reset_bus(pico_1wire_t *ctx)
{
	bool device_found = false;
	int i;

	if (!ctx)
		return device_found;

	/* Make sure power MOSFET is off (if one is present) */
	power_mosfet_off(ctx);

	/* Transmit Reset Pulse (480us minimum) */
	gpio_set_dir(ctx->data_pin, GPIO_OUT);
	gpio_put(ctx->data_pin, false);
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


int pico_1wire_read_rom(pico_1wire_t *ctx, uint64_t *addr)
{
	uint8_t crc = 0;
	uint8_t b;

	if (!ctx || !addr)
		return -1;

	/* Reset bus and check if any devices present. */
	if (!pico_1wire_reset_bus(ctx))
		return 1;

	/* Send Read ROM command */
	write_byte(ctx, CMD_READ);

	/* Read ROM Address (64bit) */
	*addr = 0;
	for (int i = 0; i < 8; i++) {
		b = read_byte(ctx);
		if (i < 7)
			crc = crc8(crc, b);
		*addr <<= 8;
		*addr |= b;
	}

	/* Check ROM checksum */
	if (b != crc)
		return 2;

	return 0;
}


int pico_1wire_search_rom(pico_1wire_t *ctx, uint64_t  *addr_list, uint addr_list_size, uint *devices_found)
{
	bool done = false;
	uint last_discrepancy = 0;
	uint64_t rom_addr = 0;


	if (!ctx || !addr_list || !devices_found || addr_list_size < 1)
		return -1;

	*devices_found = 0;
	memset(addr_list, 0, addr_list_size * sizeof(uint64_t));

	/* Reset bus and check if any devices are present. */
	if (!pico_1wire_reset_bus(ctx))
		return 1;

	while (find_next_device(ctx, &rom_addr, &done, &last_discrepancy)) {
		/* Check CRC and reverse byte order at the same time... */
		uint64_t new_addr = 0;
		uint8_t *p = &((uint8_t*)&new_addr)[7];
		uint8_t crc = 0;
		uint8_t byte;
		for (int i = 0; i < 8; i++) {
			byte = ((uint8_t*)&rom_addr)[i];
			if (i < 7)
				crc = crc8(crc, byte);
			*p-- = byte;
		}
		if (crc == byte) {
			//printf("Found device: %016llX\n", new_addr);
			if (*devices_found >= addr_list_size)
				return 2;
			addr_list[*devices_found] = new_addr;
			*devices_found = *devices_found + 1;
		} else {
			//printf("Bad CRC: %016llX\n", new_addr);
		}
	}

	return 0;
}


int pico_1wire_read_power_supply(pico_1wire_t *ctx, uint64_t addr, bool *present)
{
	if (!ctx)
		return -1;

	if (match_rom(ctx, addr))
		return 1;

	/* Send Read Power Supply command */
	write_byte(ctx, CMD_READ_POWER_SUPPLY);

	ctx->psu_present = read_bit(ctx);

	if (present)
		*present = ctx->psu_present;

	return 0;
}


int pico_1wire_read_scratch_pad(pico_1wire_t *ctx, uint64_t addr, uint8_t *buf)
{
	const uint len = 9;
	uint8_t crc = 0;

	if (!ctx || !buf)
		return -1;

	if (match_rom(ctx, addr))
		return 1;

	/* Send Read Scratch Pad command. */
	write_byte(ctx, CMD_READ_SCRATCHPAD);

	/* Read response and calculate CRC */
	for (int i = 0; i < len; i++) {
		buf[i] = read_byte(ctx);
		if (i < (len - 1))
			crc = crc8(crc, buf[i]);
	}

	/* Check CRC checksum */
	if (crc != buf[len - 1])
		return 2;

	return 0;
}


int pico_1wire_write_scratch_pad(pico_1wire_t *ctx, uint64_t addr, uint8_t *buf)
{
	if (!ctx || !buf)
		return -1;

	if (match_rom(ctx, addr))
		return 1;


	/* Send Read Scratch Pad command. */
	write_byte(ctx, CMD_WRITE_SCRATCHPAD);

	write_byte(ctx, buf[2]); /* T(H) register */
	write_byte(ctx, buf[3]); /* T(L) register */

	if (ADDR_FAMILY_CODE(addr) != FAMILY_CODE_DS18S20)
		write_byte(ctx, buf[4]); /* Configuration register */

	return 0;
}


int pico_1wire_convert_duration(pico_1wire_t *ctx, uint64_t addr, uint *duration)
{
	uint delay = MAX_TEMP_CONVERSION_TIME;
	uint8_t scratch[9];

	if (!ctx || !duration)
		return -1;

	if (addr) {
		switch(ADDR_FAMILY_CODE(addr)) {

		case FAMILY_CODE_DS18B20:
		case FAMILY_CODE_DS1822:
		case FAMILY_CODE_DS1825:
		case FAMILY_CODE_DS28EA00:
			if (!pico_1wire_read_scratch_pad(ctx, addr, scratch)) {
				uint8_t resolution = ((scratch[4] & 0x7f) >> 5) + 9;
				if (resolution == 9)
					delay = 95;
				else if (resolution == 10)
					delay = 190;
				else if (resolution == 11)
					delay = 375;
			}
			break;

		default:
			break;
		}
	}

	*duration = delay;
	return 0;
}


int pico_1wire_convert_temperature(pico_1wire_t *ctx, uint64_t addr, bool wait)
{
	uint delay = MAX_TEMP_CONVERSION_TIME;

	if (!ctx)
		return -1;

	/* Send Match ROM or Skip ROM command as needed... */
	if (match_rom(ctx, addr))
		return 1;

	/* Send Convert Temperature command. */
	write_byte(ctx, CMD_CONVERT);

	if (!ctx->psu_present)
		power_mosfet_on(ctx);

	if (wait) {
		sleep_ms(delay);
		if (!ctx->psu_present)
			power_mosfet_off(ctx);
	}

	return 0;
}


int pico_1wire_get_temperature(pico_1wire_t *ctx, uint64_t addr, float *temperature)
{
	uint8_t scratch[9];
	int temp_read;
	float temp;
	int result = 0;

	if (!ctx || !temperature)
		return -1;

	if (pico_1wire_read_scratch_pad(ctx, addr, scratch))
		return 1;

	/* Convert reading to integer */
	temp_read = (scratch[1] << 8) | scratch[0];
	if (temp_read & 0x8000)
		temp_read = - ((temp_read ^ 0xffff) + 1);


	switch(ADDR_FAMILY_CODE(addr)) {

	case FAMILY_CODE_DS1822:
	case FAMILY_CODE_DS18B20:
	case FAMILY_CODE_DS1825:
	case FAMILY_CODE_DS28EA00:
		temp =  (float)temp_read / 16.0;
		break;

	case FAMILY_CODE_DS18S20:
		int count_remain = scratch[6];
		int count_per_degree = scratch[7];
		temp = (temp_read / 2) - 0.25 + (count_per_degree - count_remain) / (float)count_per_degree;
		break;

	default:
		temp = (float)temp_read / 16.0; /* Best quess... */
		result = 2; /* Return error code on unsupported sensors. */
		break;
	}

	*temperature = temp;

	return result;
}


int pico_1wire_get_resolution(pico_1wire_t *ctx, uint64_t addr, uint *resolution)
{
	uint8_t scratch[9];

	if (!ctx || !addr || !resolution)
		return -1;

	if (pico_1wire_read_scratch_pad(ctx, addr, scratch))
		return 1;

	switch(ADDR_FAMILY_CODE(addr)) {

	case FAMILY_CODE_DS18B20:
	case FAMILY_CODE_DS1822:
	case FAMILY_CODE_DS1825:
	case FAMILY_CODE_DS28EA00:
		uint8_t res = ((scratch[4] & 0x7f) >> 5) + 9;
		//printf("config: %02x, res=%u\n", scratch[4], res);
		*resolution = res;
		break;

	case FAMILY_CODE_DS18S20:
		*resolution = 9;
		break;

	default:
		*resolution = 0;
		return 2;
	}

	return 0;
}


int pico_1wire_set_resolution(pico_1wire_t *ctx, uint64_t addr, uint resolution)
{
	uint8_t scratch[9];
	uint8_t new_cfg;

	if (!ctx || !addr || resolution < 9 || resolution > 12)
		return -1;

	if (pico_1wire_read_scratch_pad(ctx, addr, scratch))
		return 1;

	switch(ADDR_FAMILY_CODE(addr)) {

	case FAMILY_CODE_DS18B20:
	case FAMILY_CODE_DS1822:
	case FAMILY_CODE_DS1825:
	case FAMILY_CODE_DS28EA00:
		new_cfg = (scratch[4] & 0x9f) | ((resolution - 9) << 5);
		//printf("config: %02x, new config: %02x\n", scratch[4], new_cfg);
		scratch[4] = new_cfg;
		if (pico_1wire_write_scratch_pad(ctx, addr, scratch))
			return 2;
		break;

	default:
		return 3;
	}

	return 0;
}


