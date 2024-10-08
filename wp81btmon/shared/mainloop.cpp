#include "stdafx.h"

void mainloop_init(void)
{}

void mainloop_quit(void)
{}

void mainloop_exit_success(void)
{}

void mainloop_exit_failure(void)
{}

int mainloop_run(void)
{
	return 0;
}

int mainloop_run_with_signal(PHANDLER_ROUTINE HandlerRoutine, void *user_data)
{
	return 0;
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