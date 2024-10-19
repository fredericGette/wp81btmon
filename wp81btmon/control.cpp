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

#define CONTROL_DEVICE 0x8000
#define IOCTL_CONTROL_WRITE_HCI CTL_CODE(CONTROL_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_CONTROL_READ_HCI	CTL_CODE(CONTROL_DEVICE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CONTROL_CMD		CTL_CODE(CONTROL_DEVICE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// The format of a single HCI message stored in HCI_LOG_BUFFER::LogEntries.
//
#include <pshpack1.h>
typedef struct _HCI_LOG_ENTRY
{
	//
	// The system time of when this message is seen by the driver filter.
	//
	LARGE_INTEGER Timestamp;

	//
	// The length of the message stored in LogLine in characters.
	//
	USHORT LogLineLength;

	//
	// The HCI log message.
	//
	CHAR LogLine[ANYSIZE_ARRAY];
} HCI_LOG_ENTRY, *PHCI_LOG_ENTRY;
static_assert(sizeof(HCI_LOG_ENTRY) == 11, "Must be packed for space");
#include <poppack.h>

typedef struct _HCI_LOG_BUFFER
{
	ULONG NextLogOffset;
	ULONG OverflowedLogSize;
	HCI_LOG_ENTRY LogEntries[ANYSIZE_ARRAY];
} HCI_LOG_BUFFER, *PHCI_LOG_BUFFER;
static size_t hciLogBufferSize = 8 + 8 + 32768; // ULONG NextLogOffset (8) + ULONG OverflowedLogSize (8) + DEBUG_LOG_ENTRY LogEntries (32768)

static struct btsnoop *btsnoop_file = NULL;
static HANDLE hciControlDevice = NULL;
static PHCI_LOG_BUFFER pHciLogBuffer = NULL;

static int server_fd = -1;

struct control_data {
	uint16_t channel;
	HANDLE fd;
	unsigned char buf[BTSNOOP_MAX_PACKET_SIZE];
	uint16_t offset;
};

/*
Called by the mainloop
Write HCI message to the btsnoop file when this file exists.
*/
static void data_callback(HANDLE fd, uint32_t events, void *user_data)
{
	struct control_data *data = static_cast<control_data*>(user_data);
	struct timeval *tv = NULL;
	uint16_t opcode = 0 , index = 0, pktlen = 0;

	switch (data->channel) {
	case HCI_CHANNEL_CONTROL:
		packet_control(tv, NULL, index, opcode,
						data->buf, pktlen);
		break;
	case HCI_CHANNEL_MONITOR:
		btsnoop_write_hci(btsnoop_file, tv, index, opcode, 0,
						data->buf, pktlen);
		ellisys_inject_hci(tv, index, opcode,
						data->buf, pktlen);
		packet_monitor(tv, NULL, index, opcode,
						data->buf, pktlen);
		break;
	}
}

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

void control_cmd_bthx(bool blockBthx)
{
	hciControlDevice = CreateFileA("\\\\.\\wp81controldevice", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hciControlDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open wp81controldevice device! 0x%08X\n", GetLastError());
		return;
	}

	DWORD returned;
	char command[1] = { 0 };
	if (blockBthx == TRUE)
	{
		command[0] = 1;
	}
	BOOL success = DeviceIoControl(hciControlDevice, IOCTL_CONTROL_CMD, command, 1, NULL, 0, &returned, NULL);
	if (!success)
	{
		printf("Failed to send DeviceIoControl! 0x%08X", GetLastError());
	}

	CloseHandle(hciControlDevice);
}

void control_block_bthx(void)
{
	control_cmd_bthx(true);
}

void control_allow_bthx(void)
{
	control_cmd_bthx(false);
}

int control_tracing(void)
{
	packet_add_filter(PACKET_FILTER_SHOW_INDEX);

	if (server_fd >= 0)
		return 0;

	hciControlDevice = CreateFileA("\\\\.\\wp81controldevice", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hciControlDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open wp81controldevice device! 0x%08X\n", GetLastError());
		return -1;
	}

	pHciLogBuffer = (PHCI_LOG_BUFFER)malloc(hciLogBufferSize);

	if (mainloop_add_fd(hciControlDevice, 0, data_callback,	NULL, NULL) < 0) {
		free(pHciLogBuffer);
		CloseHandle(hciControlDevice);
		return -1;
	}

	mainloop_activate_tracing(packet_has_filter(PACKET_FILTER_SHOW_MGMT_SOCKET));

	return 0;
}

void *control_get_tracing(void)
{
	DWORD returned;
	ZeroMemory(pHciLogBuffer, hciLogBufferSize);
	BOOL success = DeviceIoControl(hciControlDevice, IOCTL_CONTROL_READ_HCI, NULL, 0, pHciLogBuffer, hciLogBufferSize, &returned, NULL);
	if (!success)
	{
		printf("Failed to send DeviceIoControl! 0x%08X", GetLastError());
	}

	return pHciLogBuffer;
}

static void free_tracing(void *user_data)
{
	free(pHciLogBuffer);
	CloseHandle(hciControlDevice);
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