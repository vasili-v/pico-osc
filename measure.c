#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/structs/systick.h"

#include "pico-con.h"

#include "measure.h"

// Overflow each 120ms at 125MHz
#define SYSTICK_RVR           (15000000 - 1)
#define SYSTICK_COUNTFLAG     0x00010000
#define SYSTICK_CLKSOURCE_CPU 0x00000004
#define SYSTICK_ENABLE        0x00000001
#define SYSTICK_CURRENT       0x00ffffff

enum op_states
{
	OP_STATE_IDLE,
	OP_STATE_MEASUREMENT_STARTING,
	OP_STATE_MEASUREMENT_RUNNING,
};

volatile enum op_states op_state = OP_STATE_IDLE;

#define PROBE_PIN      22

#define MIN_POINTS     1
#define MAX_POINTS     DEFAULT_POINTS

size_t data_size = DEFAULT_POINTS;
size_t data_idx = 0;
bool *data = NULL;
size_t *ticks = NULL;
size_t tick_loops = -1;

void measurement(void);

int status(size_t argc, char *argv[])
{
	switch (op_state)
	{
	case OP_STATE_IDLE:
		puts("Device is idle.");
		break;
	case OP_STATE_MEASUREMENT_STARTING:
		puts("Starting measurement.");
		break;
	case OP_STATE_MEASUREMENT_RUNNING:
		puts("Measurement in progress.");
		break;
	default:
		printf("Unknown operation state: %d.\n", op_state);
	}

	if (data && data_idx > 0)
	{
		printf("Have %lu measured points.\n", data_idx);
	}

	return PICO_CON_COMMAND_SUCCESS;
}

int show_data(size_t argc, char *argv[])
{
	if (!data || !data_idx)
	{
		puts("No data available.");
		return PICO_CON_COMMAND_SUCCESS;
	}

	if (ticks)
	{
		puts("#\tTick\tValue");
		for (size_t i = 0; i < data_size; i++)
		{
			printf("%lu\t%lu\t%hhu\n", i, ticks[i], data[i]);
		}
	}
	else
	{
		puts("#\tValue");
		for (size_t i = 0; i < data_size; i++)
		{
			printf("%lu\t%hhu\n", i, data[i]);
		}
	}
	putchar('\n');

	return PICO_CON_COMMAND_SUCCESS;
}

int measure(size_t argc, char *argv[])
{
	if (op_state != OP_STATE_IDLE)
	{
		puts("Device is busy.");
		return PICO_CON_COMMAND_SUCCESS;
	}

	data_size = DEFAULT_POINTS;
	if (argc > 0)
	{
		char *endptr = NULL;
		data_size = strtoul(argv[0], &endptr, 10);
		if (endptr && *endptr != '\0')
		{
			printf("Can't treat \"%s\" as a number of points to measure. Aborting.\n", argv[0]);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (data_size < MIN_POINTS)
		{
			printf("Requested not enough points to measure (%lu). " \
			       "Expected at least " TXT(MIN_POINTS) ". Aborting.\n", data_size);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (data_size > MAX_POINTS)
		{
			printf("Requested too many points to measure (%lu). " \
			       "Expected at most " TXT(MAX_POINTS) ". Aborting.\n", data_size);
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
		puts("Failed to allocate memory for measurement data. Aborting.");
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
		puts("Failed to allocate memory for measurement timestamps. Aborting.");
		free(data);
		data = NULL;
		return PICO_CON_COMMAND_SUCCESS;
	}

	printf("Starting measurement for %lu points.\n", data_size);

	op_state = OP_STATE_MEASUREMENT_STARTING;

	multicore_reset_core1();
	multicore_launch_core1(measurement);

	return PICO_CON_COMMAND_SUCCESS;
}

void measurement(void)
{
	op_state = OP_STATE_MEASUREMENT_RUNNING;

	gpio_init(PROBE_PIN);
	gpio_set_dir(PROBE_PIN, GPIO_IN);
	gpio_set_input_hysteresis_enabled(PROBE_PIN, 1);

	tick_loops = -1;
	tick_loops -= SYSTICK_RVR;
	systick_hw->cvr = SYSTICK_RVR;
	systick_hw->rvr = SYSTICK_RVR;
	systick_hw->csr = SYSTICK_CLKSOURCE_CPU | SYSTICK_ENABLE;

	while (data_idx < data_size)
	{
		int csr = systick_hw->csr;
		int cvr = systick_hw->cvr;
		if (csr & SYSTICK_COUNTFLAG)
		{
			tick_loops -= SYSTICK_RVR+1;
		}

		ticks[data_idx] = tick_loops + (cvr & SYSTICK_CURRENT);
		data[data_idx] = gpio_get(PROBE_PIN);
		data_idx++;
	}

	gpio_deinit(PROBE_PIN);

	op_state = OP_STATE_IDLE;

	while (1)
		tight_loop_contents();
}
