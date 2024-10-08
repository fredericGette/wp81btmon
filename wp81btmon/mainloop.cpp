#include "../stdafx.h"

void mainloop_init(void)
{}

void mainloop_quit(void)
{}

void mainloop_exit_success(void)
{}

void mainloop_exit_failure(void)
{}

int mainloop_run(void)
{}

int mainloop_run_with_signal(PHANDLER_ROUTINE HandlerRoutine, void *user_data)
{}

int mainloop_add_fd(int fd, uint32_t events, mainloop_event_func callback,
	void *user_data, mainloop_destroy_func destroy)
{}

int mainloop_modify_fd(int fd, uint32_t events)
{}

int mainloop_remove_fd(int fd)
{}

int mainloop_add_timeout(unsigned int msec, mainloop_timeout_func callback,
	void *user_data, mainloop_destroy_func destroy)
{}

int mainloop_modify_timeout(int fd, unsigned int msec)
{}

int mainloop_remove_timeout(int id)
{}

//TESTFG
//int mainloop_set_signal(sigset_t *mask, mainloop_signal_func callback,
//	void *user_data, mainloop_destroy_func destroy);

int mainloop_sd_notify(const char *state)
{}