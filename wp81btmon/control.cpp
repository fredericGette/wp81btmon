/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
*
*  BlueZ - Bluetooth protocol stack for Linux
*
*  Copyright (C) 2012-2014  Intel Corporation. All rights reserved.
*
*
*/

#include "stdafx.h"

static struct btsnoop *btsnoop_file = NULL;
static bool hcidump_fallback = false;

static int server_fd = -1;

bool control_writer(const char *path)
{
	btsnoop_file = btsnoop_create(path, 0, 0, BTSNOOP_FORMAT_MONITOR);

	return !!btsnoop_file;
}

void control_reader(const char *path, bool pager)
{
	printf("TESTFG: not implemented yet.\n");
}

void control_server(const char *path)
{
}

int control_tty(const char *path, unsigned int speed)
{
	printf("TESTFG: not implemented yet.\n");
	return 0;
}

int control_rtt(char *jlink, char *rtt)
{
	printf("TESTFG: not implemented yet.\n");
	return 0;
}

int control_tracing(void)
{
	packet_add_filter(PACKET_FILTER_SHOW_INDEX);

	if (server_fd >= 0)
		return 0;

	if (open_channel(HCI_CHANNEL_MONITOR) < 0) {
		if (!hcidump_fallback)
			return -1;
		// TESTFG
		//if (hcidump_tracing() < 0)
		//	return -1;
		return 0;
	}

	if (packet_has_filter(PACKET_FILTER_SHOW_MGMT_SOCKET))
		open_channel(HCI_CHANNEL_CONTROL);

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