// See https://github.com/bluez/bluez/blob/master/monitor/main.c
// wp81btmon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Getopt-for-Visual-Studio/getopt.h"

BOOL WINAPI consoleHandler(DWORD signal)
{
	switch (signal)
	{
	case CTRL_C_EVENT:
		mainloop_quit();
		// Signal is handled - don't pass it on to the next handler.
		return TRUE;
	default:
		// Pass signal on to the next handler.
		return FALSE;
	}
}

// used by gets_s function
void myInvalidParameterHandler(const wchar_t* expression,
	const wchar_t* function,
	const wchar_t* file,
	unsigned int line,
	uintptr_t pReserved)
{
	// do nothing
}

static void usage(void)
{
	printf("btmon - Bluetooth monitor\n"
		"Usage:\n");
	printf("\tbtmon [options]\n");
	printf("options:\n"
		"\t-b, --block            Block BTHX IoCtl\n"
		"\t-a, --allow            Allow BTHX IoCtl\n"
		"\t-e, --event            Read events\n"
		"\t-c, --command          Interactivley send commands\n"
		"\t-v, --version          Show version\n"
		"\t-h, --help             Show help options\n");
}

static const struct option main_options[] = {
	{ "block",     no_argument,       NULL, 'b' },
	{ "allow",     no_argument,       NULL, 'a' },
	{ "event",     no_argument,       NULL, 'e' },
	{ "command",   no_argument,       NULL, 'c' },
	{ "version",   no_argument,       NULL, 'v' },
	{ "help",      no_argument,       NULL, 'h' },
	{}
};

int main(int argc, char* argv[])
{
	int exit_status = EXIT_SUCCESS;
	int mainLoopMode = MODE_NOTHING;

	if (argc == 1)
	{
		fprintf(stderr, "Missing command line parameters\n");
		usage();
		return EXIT_FAILURE;
	}

	for (;;) {
		int opt;

		opt = getopt_long(argc, argv,
			"baecvh",
			main_options, NULL);

		if (opt < 0) {
			// no more option to parse
			break;
		}
			
		switch (opt) {
		case 'b':
			control_block_bthx();
			break;
		case 'a':
			control_allow_bthx();
			break;
		case 'e':
			mainLoopMode = MODE_READ_EVENTS;
			break;
		case 'c':
			mainLoopMode = MODE_SEND_COMMANDS;
			break;
		case 'v':
			printf("%s\n", VERSION);
			return EXIT_SUCCESS;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	// don't abort the application in case of invalid input with function gets_s
	_set_invalid_parameter_handler(myInvalidParameterHandler);

	if (mainLoopMode != MODE_NOTHING) {
		mainloop_init(mainLoopMode);
		mainloop_run_with_signal(consoleHandler, mainLoopMode);
		mainloop_cleanup(mainLoopMode);
	}

	return exit_status;
}

