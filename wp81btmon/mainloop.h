/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
*
*  BlueZ - Bluetooth protocol stack for Linux
*
*  Copyright (C) 2011-2014  Intel Corporation
*  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
*
*
*/

#define MODE_NOTHING 0
#define MODE_READ_EVENTS 1
#define MODE_SEND_COMMANDS 2

typedef void(*mainloop_destroy_func) (void *user_data);

typedef void(*mainloop_event_func) (HANDLE fd, uint32_t events, void *user_data);
typedef void(*mainloop_timeout_func) (int id, void *user_data);
typedef void(*mainloop_signal_func) (int signum, void *user_data);

void mainloop_init(int mainLoopMode);
void mainloop_quit(void);
void mainloop_exit_success(void);
void mainloop_exit_failure(void);
int mainloop_run(int mainLoopMode);
int mainloop_run_with_signal(PHANDLER_ROUTINE HandlerRoutine, int mainLoopMode);
void mainloop_cleanup(int mainLoopMode);

void mainloop_activate_tracing(bool channelControl);
int mainloop_add_fd(HANDLE fd, uint32_t events, mainloop_event_func callback,
	void *user_data, mainloop_destroy_func destroy);
int mainloop_modify_fd(int fd, uint32_t events);
int mainloop_remove_fd(int fd);

int mainloop_add_timeout(unsigned int msec, mainloop_timeout_func callback,
	void *user_data, mainloop_destroy_func destroy);
int mainloop_modify_timeout(int fd, unsigned int msec);
int mainloop_remove_timeout(int id);

//TESTFG
//int mainloop_set_signal(sigset_t *mask, mainloop_signal_func callback,
//	void *user_data, mainloop_destroy_func destroy);

int mainloop_sd_notify(const char *state);