#include <stdio.h>
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

void measurement(void);

int status(size_t argc, char *argv[])
{
	switch (op_state)
	{
	case OP_STATE_IDLE:
		puts("Device is idle.\n");
		break;
	case OP_STATE_MEASUREMENT_STARTING:
		puts("Starting measurement.\n");
		break;
	case OP_STATE_MEASUREMENT_RUNNING:
		puts("Measurement in progress.\n");
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
		puts("Device is busy.\n");
		return PICO_CON_COMMAND_SUCCESS;
	}
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
