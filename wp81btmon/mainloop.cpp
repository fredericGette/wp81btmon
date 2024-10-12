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

struct signal_data {
	struct io *io;
	PHANDLER_ROUTINE func;
	void *user_data;
};

#define MAX_EPOLL_EVENTS 10

static int epoll_terminate;
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

void mainloop_init(void)
{
	unsigned int i;

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++)
		mainloop_list[i] = NULL;

	epoll_terminate = 0;

	// TESTFG
	//mainloop_notify_init();
}

void mainloop_quit(void)
{
	epoll_terminate = 1;

	// TESTFG
	//mainloop_sd_notify("STOPPING=1");
}

void mainloop_exit_success(void)
{
	exit_status = EXIT_SUCCESS;
	epoll_terminate = 1;
}

void mainloop_exit_failure(void)
{
	exit_status = EXIT_FAILURE;
	epoll_terminate = 1;
}

int mainloop_run(void)
{
	unsigned int i;

	while (!epoll_terminate) {
	//	struct epoll_event events[MAX_EPOLL_EVENTS];
		int n, nfds;

	//	nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds < 0)
			continue;

		for (n = 0; n < nfds; n++) {
			struct mainloop_data *data = events[n].data.ptr;

			data->callback(data->fd, events[n].events,
				data->user_data);
		}
	}

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++) {
		struct mainloop_data *data = mainloop_list[i];

		mainloop_list[i] = NULL;

		if (data) {
			//epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);

			if (data->destroy)
				data->destroy(data->user_data);

			free(data);
		}
	}

	//close(epoll_fd);
	//epoll_fd = 0;

	// TESTFG
	//mainloop_notify_exit();

	return exit_status;
}

int mainloop_run_with_signal(PHANDLER_ROUTINE func, void *user_data)
{
	struct signal_data *data;
	int ret;

	if (!func)
		return -EINVAL;

	SetConsoleCtrlHandler(func, TRUE);

	data = new signal_data;
	data->func = func;
	data->user_data = user_data;

	ret = mainloop_run();

	free(data);

	return ret;
}

int mainloop_add_fd(int fd, uint32_t events, mainloop_event_func callback,
	void *user_data, mainloop_destroy_func destroy)
{
	return 0;
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