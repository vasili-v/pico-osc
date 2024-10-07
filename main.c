#include <stdio.h>
#include "hardware/platform_defs.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "pico-con.h"

#include "measure.h"

#if SYS_CLK_HZ != 125000000
#	error "Expecting system clock at 125MHz"
#endif

#define PROJ_DESC "RPi Pico Osc-1B"

#define INPUT_BUFFER_SIZE 63

static int show_id(size_t argc, char *argv[]);

struct pico_con_command commands[] = {
	{"id", show_id},
	{"measure", measure},
	{"data", show_data},
	{NULL, NULL},
};

int main()
{
	bi_decl(bi_program_description(PROJ_DESC));
	stdio_init_all();

	pico_con_loop(commands, INPUT_BUFFER_SIZE, MODE_BATCH);

	while(1)
		tight_loop_contents();
}

int show_id(size_t argc, char *argv[])
{
	puts(PROJ_DESC);

	return PICO_CON_COMMAND_SUCCESS;
}
