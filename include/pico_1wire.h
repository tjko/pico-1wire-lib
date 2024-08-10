/**
 * @file pico_1wire.h
 *
 * Lightweight 1-Wire Protocol C Library for Raspberry Pi Pico
 *
 * Copyright (C) 2024 Timo Kokkonen <tjko@iki.fi>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of pico-1wire Library.
 *
 * pico-1wire Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pico-1wire Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pico-1wire Library. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PICO_1WIRE_H
#define PICO_1WIRE_H 1

#include "pico/stdio.h"

#ifdef __cplusplus
extern "C"
{
#endif



/**
 * Context for 1-Wire bus instance.
 *
 * This structure stores state information about the 1-Wire bus instance.
 * Created using pico_1wire_init(), and destroyed using pico_1wire_destroy().
 */
typedef struct pico_1wire_t {
	uint data_pin;        /**< GPIO pin for 1-Wire data communications */
	uint power_pin;       /**< GPIO pin that controls MOSFET (strong pull-up) */
	bool power_available; /**< Power MOSFET available */
	bool power_state;     /**< GPIO state (1 or 0) to turn power MOSFET on */

	bool psu_present;     /**< False is one or more devices use phantom power. */
} pico_1wire_t;



/**
 * Initialize 1-Wire Bus.
 *
 * Initializes a 1-Wire bus instance. Function allocates and initializes a
 * pico_1wire_t structure (context) to use with other library functions to manipulate the bus.
 *
 * @param data_pin GPIO pin connected to 1-Wire bus data (DQ) line.
 * @param power_pin GPIO pin connected to a MOSFET that when activated acts
 *                  a strong pull-up to power devices needing phantom power.
 *                  (Set to -1 if no MOSFET available)
 * @param power_polarity Define GPIO state (1 or 0) to used to activate MOSFET
 *                       via power pin.
 *
 * @return Pointer to a new bus context allocated or NULL if function failed.
 *
 * @note When bus is initialized search check for phantom powered devices is performed.
 *       If device power status changes later, then @ref pico_1wire_read_power_supply() should
 *       be called to update power status in the bus context.
 */
pico_1wire_t* pico_1wire_init(int data_pin, int power_pin, bool power_polarity);


/**
 * Destroy previously created 1-Wire Bus context.
 *
 * This functions takes pointer to structure allocated earlier with call to pico_1wire_init().
 * GPIO lines are set back to high-imbedance (input) and resources are freed.
 *
 * @param ctx Pointer to a bus context.
 *
 * @note This function explicitly frees the context, so after call to this function
 *       context should not be referenced by the program anymore.
 */
void pico_1wire_destroy(pico_1wire_t *ctx);


/**
 * Reset 1-Wire Bus.
 *
 * This function performs reset procedure on the 1-Wire bus.
 *
 * @param ctx Pointer to a bus context.
 *
 * @return True if one or more devices are present in the Bus. False if no devices found.
 */
bool pico_1wire_reset_bus(pico_1wire_t *ctx);


/**
 * Read (ROM) Address of single device.
 *
 * This function returns ROM address of single device in the bus. This function
 * will not work if there is more than once device connected to the bus.
 *
 * @param ctx Pointer to bus context.
 * @param addr Pointer to memory variable to store found device (ROM) address.
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, bus reset failed (no devices found)
 *         - 2, ROM checksum failed (multiple device in the bus)
 *
 * @note This function is only useful if only one device is connected to the bus.
 *       Normally @ref pico_1wire_read_rom() function should be used instead.
 */
int pico_1wire_read_rom(pico_1wire_t *ctx, uint64_t *addr);


/**
 * Search (ROM) Addresses of all devices.
 *
 * This function will find all devices on the bus. Returning all (ROM) addresses
 * as well as count of devices found.
 *
 * @param ctx Pointer to bus context.
 * @param addr_list Pointer to array to store found device (ROM) addresses.
 * @param addr_list_size Size of addr_list (if bus contains more devices than this, then not all
 *                       device addresses will be returned.
 * @param devices_found Pointer to variable to store number active of devices found in the bus.
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, bus reset failed (no devices found)
 *         - 2, found more devices than addr_list_size
 */
int pico_1wire_search_rom(pico_1wire_t *ctx, uint64_t  *addr_list, uint addr_list_size, uint *devices_found);


/**
 * Read Power Supply Status of devices in the bus.
 *
 * This command is used to determine if there is one (or more) devices
 * in the bus that do not have active power supply (drawing "phantom" power from the bus).
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to check. Use 0 as address to check if all
 *             devices have power (or if there is one or more requiring phantom power).
 * @param present Pointer to variable, that reports True if all devices in the bus
 *                have power supply. If set to NULL, power supply status is not returned.
 *
 * @note This function updates power supply status saved in the bus context
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, bus reset failed (no devices found)
 */
int pico_1wire_read_power_supply(pico_1wire_t *ctx, uint64_t addr,  bool *present);


/**
 * Read Device Scratchpad (Memory)
 *
 * This function reads (Temperature) sensor (scratchpad) memory.
 * Scratch pad is 9 bytes long buffer (9th byte being CRC-8 checksum).
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param buf Buffer to store the scratchpad memory read from the device (must be at least 9 bytes long)
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, device not found
 *         - 2, bad checksum
 */
int pico_1wire_read_scratch_pad(pico_1wire_t *ctx,  uint64_t addr, uint8_t *buf);


/**
 * Write Device Scratchpad (Memory)
 *
 * This function writes (Temperature) sensor (scratchpad) memory.
 * Scratch pad is 9 bytes long buffer (9th byte being CRC-8 checksum).
 * This function always ignores the 9th byte.
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param buf Buffer that contains scratchpad memory to write to the device (must be at least 9 bytes long)
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, device not found
 *         - 2, bad checksum
 */
int pico_1wire_write_scratch_pad(pico_1wire_t *ctx, uint64_t addr, uint8_t *buf);


/**
 * Return (estimated) duration for temperature conversion process.
 *
 * This function attempts to determine how long "Convert Temperature" command will take.
 * This allows program to issue "convert" command to initiate temperature measurement.
 * And then do other things while waiting measurement to complete and calling @ref pico_1wire_get_temperature()
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param duration Pointer to variable to store estimated duration of temperature conversion (measurement).
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 */
int pico_1wire_convert_duration(pico_1wire_t *ctx, uint64_t addr, uint *duration);


/**
 * Initiate Temperature Conversion (Measurement) on one or more sensors.
 *
 * This function is used to issue "Convert Temperature" command that will initiate
 * temperature measurement. Measurement can take up to 750ms, while in some cases it can
 * be faster. @ref pico_1wire_convert_duration() can be used to estimate how long conversion
 * will take on given sensor.
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param wait When true, function does not return until conversion is complete.
 *             (Otherwise function returns immediately).
  *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, no device(s) found
 */
int pico_1wire_convert_temperature(pico_1wire_t *ctx, uint64_t addr, bool wait);


/**
 * Retrieve last temperature measurement from a sensor.
 *
 * This function will read temperature sensor "scratch pad" memory. And
 * return latest temperature measuremt in Celcius degrees.
 * @ref pico_1wire_convert_temperature() must be called before first to initiate
 * temperature conversion (measurement).
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param temperature Pointer to variable to store the temperatue (in Celcius).
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, no device found
 *         - 2, unsupported device (temperature result may be inaccurate)
 */
int pico_1wire_get_temperature(pico_1wire_t *ctx, uint64_t addr, float *temperature);


/**
 * Get current temperature measurement resolution.
 *
 * This allows checking currently configured temperature measurement resolution (9-12bits).
 * This function reads sensor configuration register to determine the currently active setting.
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param resolution Pointer to variable to store current measurement resolution (9..12)
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, no device found
 *         - 2, unsupported device (temperature result may be inaccurate)
 */
int pico_1wire_get_resolution(pico_1wire_t *ctx, uint64_t addr, uint *resolution);


/**
 * Set current temperature measurement resolution.
 *
 * This allows configuring temperature measurement resolution (9-12bit) on sernsors
 * that support variable reolutions.
 *
 * @param ctx Pointer to bus context.
 * @param addr ROM Address of the device to read.
 * @param resolution Measurement resolution (valid range from 9 to 12)
 *
 * @return Status code,
 *         - -1, invalid parameters
 *         - 0, success
 *         - 1, no device found
 *         - 2, failed to update device configuration register
 *         - 3, unsupported device
 */
int pico_1wire_set_resolution(pico_1wire_t *ctx, uint64_t addr, uint resolution);


#ifdef __cplusplus
}
#endif

#endif /* PICO_1WIRE_H */
