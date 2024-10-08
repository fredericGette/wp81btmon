#include "stdafx.h"

bool control_writer(const char *path)
{
	return false;
}

void control_reader(const char *path, bool pager)
{
}

void control_server(const char *path)
{
}

int control_tty(const char *path, unsigned int speed)
{
	return 0;
}

int control_rtt(char *jlink, char *rtt)
{
	return 0;
}

int control_tracing(void)
{
	return 0;
}

void control_disable_decoding(void)
{
}

void control_filter_index(uint16_t index)
{
}

void control_message(uint16_t opcode, const void *data, uint16_t size)
{
}