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

static void usage(void)
{
	printf("btmon - Bluetooth monitor\n"
		"Usage:\n");
	printf("\tbtmon [options]\n");
	printf("options:\n"
		"\t-b, --block            Block BTHX IoCtl\n"
		"\t-a, --allow            Allow BTHX IoCtl\n"
		"\t-v, --version          Show version\n"
		"\t-h, --help             Show help options\n");
}

static const struct option main_options[] = {
	{ "block",     no_argument,       NULL, 'b' },
	{ "allow",     no_argument,       NULL, 'a' },
	{ "version",   no_argument,       NULL, 'v' },
	{ "help",      no_argument,       NULL, 'h' },
	{}
};

int main(int argc, char* argv[])
{
	int exit_status = EXIT_SUCCESS;

	if (argc == 1)
	{
		fprintf(stderr, "Missing command line parameters\n");
		usage();
		return EXIT_FAILURE;
	}

	for (;;) {
		int opt;

		opt = getopt_long(argc, argv,
			"bavh",
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

	return exit_status;
}

