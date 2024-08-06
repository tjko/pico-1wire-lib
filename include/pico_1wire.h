/* pico_1wire.h
   Copyright (C) 2024 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of pico-1wire Library.

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

#ifndef PICO_1WIRE_H
#define PICO_1WIRE_H 1

#include "pico/stdio.h"


typedef struct pico_1wire_t {
	uint data_pin;
	uint power_pin;
	bool power_available; /* Power MOSFET available */
	bool power_state; /* GPIO state to turn power MOSFET on */

	bool psu_present;
} pico_1wire_t;



pico_1wire_t* pico_1wire_init(int data_pin, int power_pin, bool power_polarity);
void pico_1wire_destroy(pico_1wire_t *ctx);

bool pico_1wire_reset_bus(pico_1wire_t *ctx);
int pico_1wire_read_rom(pico_1wire_t *ctx, uint64_t *addr);
int pico_1wire_search_rom(pico_1wire_t *ctx, uint64_t  *addr_list, uint addr_list_size, uint *devices_found);
int pico_1wire_read_power_supply(pico_1wire_t *ctx,  bool *present);
int pico_1wire_read_scratch_pad(pico_1wire_t *ctx,  uint64_t addr, uint8_t *buf);
int pico_1wire_write_scratch_pad(pico_1wire_t *ctx, uint64_t addr, uint8_t *buf);

int pico_1wire_convert_temperature(pico_1wire_t *ctx, uint64_t addr, bool wait);
int pico_1wire_get_temperature(pico_1wire_t *ctx, uint64_t addr, float *temperature);
int pico_1wire_get_resolution(pico_1wire_t *ctx, uint64_t addr, uint *resolution);
int pico_1wire_set_resolution(pico_1wire_t *ctx, uint64_t addr, uint resolution);


#endif /* PICO_1WIRE_H */
