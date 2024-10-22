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
#define IOCTL_CONTROL_WRITE_HCI CTL_CODE(CONTROL_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x80002003
#define IOCTL_CONTROL_READ_HCI	CTL_CODE(CONTROL_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x80002007
#define IOCTL_CONTROL_CMD		CTL_CODE(CONTROL_DEVICE, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x8000200B

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

UCHAR* inputBuffer;
UCHAR* outputBuffer;

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
	inputBuffer = (UCHAR*)malloc(1);
	inputBuffer[0] = 0;
	if (blockBthx == TRUE)
	{
		inputBuffer[0] = 1;
	}
	BOOL success = DeviceIoControl(hciControlDevice, IOCTL_CONTROL_CMD, inputBuffer, 1, NULL, 0, &returned, NULL);
	if (!success)
	{
		printf("Failed to send DeviceIoControl! 0x%08X", GetLastError());
	}

	CloseHandle(hciControlDevice);
	free(inputBuffer);
}

void control_block_bthx(void)
{
	control_cmd_bthx(true);
}

void control_allow_bthx(void)
{
	control_cmd_bthx(false);
}

void control_init_read_events(void)
{
	hciControlDevice = CreateFileA("\\\\.\\wp81controldevice", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hciControlDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open wp81controldevice device! 0x%08X\n", GetLastError());
		abort();
	}

	inputBuffer = (UCHAR*)malloc(4);
	outputBuffer = (UCHAR*)malloc(262);

	printf("Start reading HCI events...\n");
}

void control_cleanup_read_events(void)
{
	CloseHandle(hciControlDevice);
	free(inputBuffer);
	free(outputBuffer);
}

void printBuffer2HexString(UCHAR* buffer, size_t bufSize)
{
	FILETIME SystemFileTime;
	UCHAR *p = buffer;
	UINT i = 0;

	if (bufSize < 1)
	{
		return;
	}

	GetSystemTimeAsFileTime(&SystemFileTime);
	printf("%010u.%010u ", SystemFileTime.dwHighDateTime, SystemFileTime.dwLowDateTime);
	for (; i<bufSize; i++)
	{
		printf("%02X ", p[i]);
	}
	printf("\n");
}

bool control_read_events(void)
{
	DWORD information = 0;
	bool success;

	inputBuffer[0] = 0x04;
	inputBuffer[1] = 0x00;
	inputBuffer[2] = 0x00;
	inputBuffer[3] = 0x00;
	success = DeviceIoControl(hciControlDevice, IOCTL_CONTROL_READ_HCI, inputBuffer, 4, outputBuffer, 262, &information, nullptr);
	if (success)
	{
		printBuffer2HexString(outputBuffer, information);
	}
	else
	{
		printf("Failed to send IOCTL_CONTROL_READ_HCI! 0x%X\n", GetLastError());
	}

	return success;
}

void control_init_send_commands(void)
{
	hciControlDevice = CreateFileA("\\\\.\\wp81controldevice", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hciControlDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open wp81controldevice device! 0x%08X\n", GetLastError());
		abort();
	}

	inputBuffer = (UCHAR*)malloc(262);
	outputBuffer = (UCHAR*)malloc(4);

	printf("Input HCI commands...\n");
}

void control_cleanup_send_commands(void)
{
	CloseHandle(hciControlDevice);
	free(inputBuffer);
	free(outputBuffer);
}

bool control_send_commands(void)
{
	DWORD information = 0;
	bool success;
	char line[262*3+1];

	printf(">");
	if (gets_s(line, 262 * 3) == NULL) {
		printf("Invalid input. Max length is %d characters.\n", 262 * 3);
		return TRUE; // try again.
	}

	DWORD value = 0;
	DWORD nbOfDigit = 0;
	DWORD inputBufferLength = 0;
	for (DWORD i = 0; i < strlen(line); i++) {
		char c = line[i];
		if (c >= '0' && c <= '9') {
			value += c - '0';
			nbOfDigit++;
		} 
		else if (c >= 'A' && c <= 'F') {
			value += c - 'A' + 10;
			nbOfDigit++;
		}
		else if (c >= 'a' && c <= 'f') {
			value += c - 'a' + 10;
			nbOfDigit++;
		}
		else if (nbOfDigit != 0) {
			printf("Invalid input. Must be a list of bytes in hexadecimal.\n", 262 * 3);
			return TRUE; // try again.
		}

		if (nbOfDigit == 1) {
			value = value << 4;
		} 
		else if (nbOfDigit == 2) {
			inputBuffer[inputBufferLength++] = value;
			value = 0;
			nbOfDigit = 0;
		}

		if (inputBufferLength > 262) {
			printf("Invalid input. Max number of bytes is %d.\n", 262);
			return TRUE; // try again.
		}
	}
	if (nbOfDigit != 0) {
		printf("Invalid input. Must be a list of bytes in hexadecimal.\n", 262 * 3);
		return TRUE; // try again.
	}

	// Inquiry
	// 08 00 00 00 01 01 04 05 33 8B 9E 08 00
	printBuffer2HexString(inputBuffer, inputBufferLength);
	success = DeviceIoControl(hciControlDevice, IOCTL_CONTROL_WRITE_HCI, inputBuffer, inputBufferLength, outputBuffer, 4, &information, nullptr);
	if (success)
	{
		printBuffer2HexString(outputBuffer, information);
	}
	else
	{
		printf("Failed to send IOCTL_CONTROL_WRITE_HCI! 0x%X\n", GetLastError());
	}

	return success;
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