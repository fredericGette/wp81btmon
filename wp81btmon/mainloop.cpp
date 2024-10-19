// SPDX-License-Identifier: LGPL-2.1-or-later
/*
*
*  BlueZ - Bluetooth protocol stack for Linux
*
*  Copyright (C) 2011-2014  Intel Corporation
*  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
*
*
*/

#include "stdafx.h"

static int mainloop_terminate;
static bool tracingChannelMonitor;
static bool tracingChannelControl;
static int exit_status = EXIT_SUCCESS;

struct mainloop_data {
	HANDLE fd;
	uint32_t events;
	mainloop_event_func callback;
	mainloop_destroy_func destroy;
	void *user_data;
};

#define MAX_MAINLOOP_ENTRIES 128

static struct mainloop_data *mainloop_list[MAX_MAINLOOP_ENTRIES];
static int mainloop_list_size;

void mainloop_init(void)
{
	unsigned int i;

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++)
		mainloop_list[i] = NULL;
	mainloop_list_size = 0;

	mainloop_terminate = 0;
	tracingChannelMonitor = FALSE;
	tracingChannelControl = FALSE;

	// TESTFG
	//mainloop_notify_init();
}

void mainloop_quit(void)
{
	mainloop_terminate = 1;

	// TESTFG
	//mainloop_sd_notify("STOPPING=1");
}

void mainloop_exit_success(void)
{
	exit_status = EXIT_SUCCESS;
	mainloop_terminate = 1;
}

void mainloop_exit_failure(void)
{
	exit_status = EXIT_FAILURE;
	mainloop_terminate = 1;
}

int mainloop_run(void)
{
	unsigned int i;

	while (!mainloop_terminate) {
		int n;
		void *user_data = NULL;

		if (tracingChannelMonitor)
		{
			user_data = control_get_tracing();
		}

		for (n = 0; n < mainloop_list_size; n++) {
			struct mainloop_data *data = mainloop_list[n];

			data->callback(data->fd, 0, user_data);
		}
	}

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++) {
		struct mainloop_data *data = mainloop_list[i];

		mainloop_list[i] = NULL;

		if (data) {

			if (data->destroy)
				data->destroy(data->user_data);

			free(data);
		}
	}

	// TESTFG
	//mainloop_notify_exit();

	return exit_status;
}

int mainloop_run_with_signal(PHANDLER_ROUTINE func, void *user_data)
{
	int ret;

	if (!func)
		return -EINVAL;

	SetConsoleCtrlHandler(func, TRUE);

	ret = mainloop_run();

	return ret;
}

int mainloop_add_fd(HANDLE fd, uint32_t events, mainloop_event_func callback,
	void *user_data, mainloop_destroy_func destroy)
{
	struct mainloop_data *data;

	if (mainloop_list_size >= MAX_MAINLOOP_ENTRIES - 1)
		return -EINVAL;

	data = (mainloop_data *)malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->fd = fd;
	data->events = events;
	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	mainloop_list[mainloop_list_size++] = data;

	return 0;
}

// Activate reading from the local driver filter
void mainloop_activate_tracing(bool channelControl)
{
	tracingChannelMonitor = TRUE;
	tracingChannelControl = channelControl;
}


int mainloop_modify_fd(int fd, uint32_t events)
{
	return 0;
}

int mainloop_remove_fd(int fd)
{
	return 0;
}

int mainloop_add_timeout(unsigned int msec, mainloop_timeout_func callback,
	void *user_data, mainloop_destroy_func destroy)
{
	return 0;
}

int mainloop_modify_timeout(int fd, unsigned int msec)
{
	return 0;
}

int mainloop_remove_timeout(int id)
{
	return 0;
}

//TESTFG
//int mainloop_set_signal(sigset_t *mask, mainloop_signal_func callback,
//	void *user_data, mainloop_destroy_func destroy);

int mainloop_sd_notify(const char *state)
{
	return 0;
}