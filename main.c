#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "pico-con.h"

#define SYSTICK_RVR 0xE4E1BF

#define PROJ_DESC "A 1-bit oscilloscope for Raspberry Pi Pico"

#define INPUT_BUFFER_SIZE 255

enum op_states
{
	OP_STATE_IDLE,
};

volatile enum op_states op_state = OP_STATE_IDLE;

static void banner(void);
static int status(size_t argc, char *argv[]);
static int help(size_t argc, char *argv[]);

struct pico_con_command commands[] = {
	{"status", status},
	{"help", help},
	{NULL, NULL},
};

int main()
{
	bi_decl(bi_program_description(PROJ_DESC));
	stdio_init_all();
	banner();

	puts(pico_con_loop(commands, INPUT_BUFFER_SIZE)? \
	     "Failed to run serial console.\n" :         \
	     "Serial console closed.\n");

	while(1)
		tight_loop_contents();
}

void banner(void)
{
	for (int i = 0; i < 10; i++)
	{
		sleep_ms(500);
		putchar('.');
	}
	puts("\n" PROJ_DESC "\n");
}

int status(size_t argc, char *argv[])
{
	switch (op_state)
	{
	case OP_STATE_IDLE:
		puts("Device is idle.\n");
		break;
	default:
		printf("Unknown operation state: %d.\n", op_state);
	}

	return PICO_CON_COMMAND_SUCCESS;
}

int help(size_t argc, char *argv[])
{
	printf("Available commands:\n" \
	       "\tstatus - prints status of current operation\n" \
	       "\thelp   - prints this message\n\n");
	return PICO_CON_COMMAND_SUCCESS;
}
