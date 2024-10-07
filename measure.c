#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"

#include "pico-con.h"

#include "measure.h"

// Overflow each 120ms at 125MHz
#define SYSTICK_COUNTFLAG     0x00010000
#define SYSTICK_CLKSOURCE_CPU 0x00000004
#define SYSTICK_ENABLE        0x00000001
#define SYSTICK_CURRENT       0x00ffffff

#define SYSTICK_LOOP          15000000UL
#define SYSTICK_RVR           (SYSTICK_LOOP - 1)
#define SYSTICK_FULL_LOOPS_32 286UL
#define SYSTICK_FULL_TICKS_32 (SYSTICK_FULL_LOOPS_32*SYSTICK_LOOP)
#define SYSTICK_FIRST_LOOP_32 (0xffffffff - SYSTICK_FULL_LOOPS_32*SYSTICK_LOOP)

#define PROBE_PIN      22

#define MIN_POINTS     1
#define MAX_POINTS     DEFAULT_POINTS

size_t data_size = DEFAULT_POINTS;
size_t data_idx = 0;
uint8_t *data = NULL;
size_t *ticks = NULL;

void measurement(void);

int show_data(size_t argc, char *argv[])
{
	if (data && data_idx)
	{
		if (ticks)
		{
			for (size_t i = 0; i < data_idx; i++)
			{
				printf("%lu,%lu,%hhu\n", i, ticks[i], data[i]);
			}
		}
		else
		{
			for (size_t i = 0; i < data_idx; i++)
			{
				printf("%lu,%hhu\n", i, data[i]);
			}
		}
	}

	return PICO_CON_COMMAND_SUCCESS;
}

int measure(size_t argc, char *argv[])
{
	data_size = DEFAULT_POINTS;
	if (argc > 0)
	{
		char *endptr = NULL;
		data_size = strtoul(argv[0], &endptr, 10);
		if (endptr && *endptr != '\0')
		{
			printf("ERR: Can't treat \"%s\" as a number of points to measure\n", argv[0]);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (data_size < MIN_POINTS)
		{
			printf("ERR: Requested not enough points to measure (%lu). " \
			       "Expected at least " TXT(MIN_POINTS) "\n", data_size);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (data_size > MAX_POINTS)
		{
			printf("ERR: Requested too many points to measure (%lu). " \
			       "Expected at most " TXT(MAX_POINTS) "\n", data_size);
			return PICO_CON_COMMAND_SUCCESS;
		}
	}

	data_idx = 0;

	if (data)
	{
		free(data);
		data = NULL;
	}
	data = malloc(data_size);
	if (!data)
	{
		puts("ERR: Failed to allocate memory for measurement data");
		return PICO_CON_COMMAND_SUCCESS;
	}

	if (ticks)
	{
		free(ticks);
		ticks = NULL;
	}
	ticks = malloc(data_size*sizeof(size_t));
	if (!ticks)
	{
		puts("ERR: Failed to allocate memory for measurement timestamps");
		free(data);
		data = NULL;
		return PICO_CON_COMMAND_SUCCESS;
	}

	printf("START: %lu\n", data_size);

	measurement();

	printf("DONE: %lu\n", data_idx);

	return PICO_CON_COMMAND_SUCCESS;
}

void measurement(void)
{
	gpio_init(PROBE_PIN);
	gpio_set_dir(PROBE_PIN, GPIO_IN);
	gpio_set_input_hysteresis_enabled(PROBE_PIN, 1);

	size_t tick_curr_base = SYSTICK_FULL_TICKS_32;
	size_t tick_prev = SYSTICK_FIRST_LOOP_32;

	uint8_t value_prev = gpio_get(PROBE_PIN)? 1 : 0;

	systick_hw->cvr = SYSTICK_FIRST_LOOP_32;
	systick_hw->rvr = SYSTICK_RVR;
	systick_hw->csr = SYSTICK_CLKSOURCE_CPU | SYSTICK_ENABLE;

	while (data_idx < data_size)
	{
		size_t tick_curr = systick_hw->cvr & SYSTICK_CURRENT;
		if (tick_curr >= tick_prev)
		{
			if (tick_curr_base <= 0)
			{
				break;
			}

			tick_curr_base -= SYSTICK_LOOP;
		}
		tick_prev = tick_curr;

		size_t tick = tick_curr_base + tick_curr;

		uint8_t value_curr = gpio_get(PROBE_PIN)? 1 : 0;
		if (value_curr != value_prev)
		{
			data[data_idx] = (value_prev << 1) | value_curr;
			ticks[data_idx] = tick;
			data_idx++;

			value_prev = value_curr;
		}
	}

	systick_hw->csr = 0;

	gpio_deinit(PROBE_PIN);
}
