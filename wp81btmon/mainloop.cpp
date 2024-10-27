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

void mainloop_init(int mainLoopMode)
{
	mainloop_terminate = 0;

	switch (mainLoopMode) {
	case MODE_READ_EVENTS:
		control_init_read_events();
		break;
	case MODE_SEND_COMMANDS:
		control_init_send_commands();
		break;
	case MODE_READ_DATA:
		control_init_read_data();
		break;
	}
}

void mainloop_cleanup(int mainLoopMode)
{
	switch (mainLoopMode) {
	case MODE_READ_EVENTS:
		control_cleanup_read_events();
		break;
	case MODE_SEND_COMMANDS:
		control_cleanup_send_commands();
		break;
	case MODE_READ_DATA:
		control_cleanup_read_data();
		break;
	}
}

void mainloop_quit(void)
{
	printf("Terminating...\n");
	mainloop_terminate = 1;
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

int mainloop_run(int mainLoopMode)
{

	while (!mainloop_terminate) {
	
		switch (mainLoopMode) {
		case MODE_READ_EVENTS:
			if (!control_read_events())
			{
				mainloop_exit_failure();
			}
			break;
		case MODE_SEND_COMMANDS:
			if (!control_send_commands())
			{
				mainloop_exit_failure();
			}
			break;
		case MODE_READ_DATA:
			if (!control_read_data())
			{
				mainloop_exit_failure();
			}
			break;
		}
	}

	return exit_status;
}

int mainloop_run_with_signal(PHANDLER_ROUTINE func, int mainLoopMode)
{
	int ret;

	if (!func)
		return -EINVAL;

	SetConsoleCtrlHandler(func, TRUE);

	ret = mainloop_run(mainLoopMode);

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