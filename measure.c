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

#define MIN_POINTS     1
#define MAX_POINTS     DEFAULT_POINTS

size_t points;

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

	return PICO_CON_COMMAND_SUCCESS;
}

int measure(size_t argc, char *argv[])
{
	if (op_state != OP_STATE_IDLE)
	{
		puts("Device is busy.");
		return PICO_CON_COMMAND_SUCCESS;
	}

	points = DEFAULT_POINTS;
	if (argc > 0)
	{
		char *endptr = NULL;
		points = strtoul(argv[0], &endptr, 10);
		if (endptr && *endptr != '\0')
		{
			printf("Can't treat \"%s\" as a number of points to measure. Aborting.\n", argv[0]);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (points < MIN_POINTS)
		{
			printf("Requested not enough points to measure (%lu). " \
			       "Expected at least " TXT(MIN_POINTS) ". Aborting.\n", points);
			return PICO_CON_COMMAND_SUCCESS;
		}

		if (points > MAX_POINTS)
		{
			printf("Requested too many points to measure (%lu). " \
			       "Expected at most " TXT(MAX_POINTS) ". Aborting.\n", points);
			return PICO_CON_COMMAND_SUCCESS;
		}
	}

	printf("Starting measurement for %lu points.\n", points);

	op_state = OP_STATE_MEASUREMENT_STARTING;

	multicore_reset_core1();
	multicore_launch_core1(measurement);

	return PICO_CON_COMMAND_SUCCESS;
}

void measurement(void)
{
	op_state = OP_STATE_MEASUREMENT_RUNNING;

	sleep_ms(10000);

	op_state = OP_STATE_IDLE;

	while (1)
		tight_loop_contents();
}
