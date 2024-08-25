#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "pico-con.h"

#include "measure.h"

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

	printf("%lu:", data_size);
	for (size_t i = 0; i < data_size; i++)
	{
		printf(" %hhu", data[i]);
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

	if (data)
	{
		free(data);
		data = NULL;
	}
	data_idx = 0;
	data = malloc(data_size);
	if (!data)
	{
		puts("Failed to allocate memory for measurement data. Aborting.");
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

	while (data_idx < data_size)
	{
		data[data_idx] = gpio_get(PROBE_PIN);
		data_idx++;
	}

	gpio_deinit(PROBE_PIN);

	op_state = OP_STATE_IDLE;

	while (1)
		tight_loop_contents();
}
