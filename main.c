#include <stdio.h>
#include "hardware/platform_defs.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "pico-con.h"

#include "measure.h"

#if SYS_CLK_HZ != 125000000
#	error "Expecting system clock at 125MHz"
#endif

#define PROJ_DESC "A 1-bit oscilloscope for Raspberry Pi Pico"

#define INPUT_BUFFER_SIZE 255

static void banner(void);
static int help(size_t argc, char *argv[]);

struct pico_con_command commands[] = {
	{"status", status},
	{"measure", measure},
	{"data", show_data},
	{"help", help},
	{NULL, NULL},
};

int main()
{
	bi_decl(bi_program_description(PROJ_DESC));
	stdio_init_all();
	banner();

	puts(pico_con_loop(commands, INPUT_BUFFER_SIZE, MODE_HUMAN_READABLE)? \
	     "Failed to run serial console." :           \
	     "Serial console closed.");

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

int help(size_t argc, char *argv[])
{
	printf("Available commands:\n" \
	       "\tstatus      - prints status of current operation\n" \
	       "\tmeasure [n] - starts a measurement routine for n points (default " TXT(DEFAULT_POINTS) ")\n" \
	       "\tdata        - prints currently available data\n" \
	       "\thelp        - prints this message\n\n");
	return PICO_CON_COMMAND_SUCCESS;
}
