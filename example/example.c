#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico_1wire.h"


#define DATA_PIN 16
#define POWER_PIN -1

#define MAX_DEVICES 32

void log_msg(const char *format, ...)
{
	va_list ap;
	char buf[256];
	int len;
	static uint64_t last_t = 0;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	if ((len = strlen(buf)) > 0) {
		/* If string ends with \n, remove it. */
		if (buf[len - 1] == '\n')
			buf[len - 1] = 0;
	}

	uint64_t t = to_us_since_boot(get_absolute_time());
	printf("[%6llu.%06llu][%8llu] %s\n", (t / 1000000), (t % 1000000), (t - last_t) / 1000, buf);
	last_t = t;
}



int main() {
	uint64_t addr;
	bool psu = false;
	uint64_t addr_list[MAX_DEVICES];
	uint device_count;
	int res;

	stdio_init_all();

	sleep_ms(250);
	printf("\n\n\nBOOT\n");

	pico_1wire_t *ctx = pico_1wire_init(DATA_PIN, POWER_PIN, true);

	if (!ctx) {
		log_msg("pico_1wire_init() failed");
		panic("halt");
	}

	log_msg("Checking for devices in the bus...");
	while (!pico_1wire_reset_bus(ctx)) {
		log_msg("No device(s) found!");
		sleep_ms(1000);
	}
	log_msg("1 or more devices detected.");

	log_msg("Checking for devices using phantom power...");
	while (pico_1wire_read_power_supply(ctx, &psu)) {
		log_msg("Read Power Supply Command failed.");
		sleep_ms(1000);
	}
	if (psu)
		log_msg("No devices using phantom power found.");
	else
		log_msg("1 or more devices using phantom power.");

	log_msg("start loop");

	while (1) {
		log_msg("Send Read ROM Command...");
		res = pico_1wire_read_rom(ctx, &addr);
		if (!res) {
			log_msg("1 Device found: %016llX", addr);
		} else {
			log_msg("Read ROM Failed (multiple devices in the bus?)");
		}

		log_msg("Find devices in the bus...");
		log_msg("search result: %d", pico_1wire_search_rom(ctx, addr_list, MAX_DEVICES, &device_count));
		log_msg("device count: %u", device_count);
		for (int i = 0; i < device_count; i++) {
			log_msg("Device %02d: %016llX\n", i + 1, addr_list[i]);
		}

		log_msg("sleep...");
		sleep_ms(10000);
	}
	return 0;
}


