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
} pico_1wire_t;



pico_1wire_t* pico_1wire_init(int data_pin, int power_pin, bool power_polarity);
void pico_1wire_destroy(pico_1wire_t *ctx);
bool pico_1wire_reset_bus(pico_1wire_t *ctx);


#endif /* PICO_1WIRE_H */
