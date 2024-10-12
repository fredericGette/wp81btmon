// SPDX-License-Identifier: LGPL-2.1-or-later
/*
*
*  BlueZ - Bluetooth protocol stack for Linux
*
*  Copyright (C) 2012-2014  Intel Corporation. All rights reserved.
*
*
*/

#include "stdafx.h"

#pragma pack(push, 1)
struct btsnoop_hdr {
	uint8_t		id[8];		/* Identification Pattern */
	uint32_t	version;	/* Version Number = 1 */
	uint32_t	type;		/* Datalink Type */
};
#pragma pack(pop)
#define BTSNOOP_HDR_SIZE (sizeof(struct btsnoop_hdr))

#pragma warning(disable : 4200)
#pragma pack(push, 1)
struct btsnoop_pkt {
	uint32_t	size;		/* Original Length */
	uint32_t	len;		/* Included Length */
	uint32_t	flags;		/* Packet Flags */
	uint32_t	drops;		/* Cumulative Drops */
	uint64_t	ts;		/* Timestamp microseconds */
	uint8_t		data[0];	/* Packet Data */
};
#pragma pack(pop)
#define BTSNOOP_PKT_SIZE (sizeof(struct btsnoop_pkt))

static const uint8_t btsnoop_id[] = { 0x62, 0x74, 0x73, 0x6e,
0x6f, 0x6f, 0x70, 0x00 };

static const uint32_t btsnoop_version = 1;

#pragma pack(push, 1)
struct pklg_pkt {
	uint32_t	len;
	uint64_t	ts;
	uint8_t		type;
};
#pragma pack(pop)
#define PKLG_PKT_SIZE (sizeof(struct pklg_pkt))

struct btsnoop {
	std::atomic_int ref_count;
	HANDLE fd;
	unsigned long flags;
	uint32_t format;
	uint16_t index;
	bool aborted;
	bool pklg_format;
	bool pklg_v2;
	const char *path;
	size_t max_size;
	size_t cur_size;
	unsigned int max_count;
	unsigned int cur_count;
};

struct btsnoop *btsnoop_create(const char *path, size_t max_size,
	unsigned int max_count, uint32_t format)
{
	struct btsnoop *btsnoop;
	struct btsnoop_hdr hdr;
	const char *real_path;
	char tmp[MAX_PATH];
	DWORD written;

	if (!max_size && max_count)
		return NULL;

	btsnoop = static_cast<struct btsnoop*>(calloc(1, sizeof(*btsnoop)));
	if (!btsnoop)
		return NULL;

	/* If max file size is specified, always add counter to file path */
	if (max_size) {
		_snprintf_s(tmp, MAX_PATH, "%s.0", path);
		real_path = tmp;
	}
	else {
		real_path = path;
	}

	btsnoop->fd = CreateFileA(real_path, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (btsnoop->fd == INVALID_HANDLE_VALUE) {
		free(btsnoop);
		return NULL;
	}

	btsnoop->format = format;
	btsnoop->index = 0xffff;
	btsnoop->path = path;
	btsnoop->max_count = max_count;
	btsnoop->max_size = max_size;

	memcpy(hdr.id, btsnoop_id, sizeof(btsnoop_id));
	hdr.version = htobe32(btsnoop_version);
	hdr.type = htobe32(btsnoop->format);

	if (FALSE == WriteFile(btsnoop->fd, &hdr, BTSNOOP_HDR_SIZE, &written, NULL)) {
		CloseHandle(btsnoop->fd);
		free(btsnoop);
		return NULL;
	}

	btsnoop->cur_size = BTSNOOP_HDR_SIZE;

	return btsnoop_ref(btsnoop);
}

struct btsnoop *btsnoop_ref(struct btsnoop *btsnoop)
{
	if (!btsnoop)
		return NULL;

	std::atomic_fetch_add(&btsnoop->ref_count, 1);

	return btsnoop;
}

static bool btsnoop_rotate(struct btsnoop *btsnoop)
{
	struct btsnoop_hdr hdr;
	char path[MAX_PATH];
	DWORD written;

	CloseHandle(btsnoop->fd);

	/* Check if max number of log files has been reached */
	if (btsnoop->max_count && btsnoop->cur_count >= btsnoop->max_count) {
		_snprintf_s(path, MAX_PATH, "%s.%u", btsnoop->path,
			btsnoop->cur_count - btsnoop->max_count);
		remove(path);
	}

	_snprintf_s(path, MAX_PATH, "%s.%u", btsnoop->path, btsnoop->cur_count);
	btsnoop->cur_count++;

	btsnoop->fd = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (btsnoop->fd == INVALID_HANDLE_VALUE) {
		return false;
	}

	memcpy(hdr.id, btsnoop_id, sizeof(btsnoop_id));
	hdr.version = htobe32(btsnoop_version);
	hdr.type = htobe32(btsnoop->format);

	if (FALSE == WriteFile(btsnoop->fd, &hdr, BTSNOOP_HDR_SIZE, &written, NULL)) {
		return false;
	}

	btsnoop->cur_size = BTSNOOP_HDR_SIZE;

	return true;
}

bool btsnoop_write(struct btsnoop *btsnoop, struct timeval *tv, uint32_t flags,
	uint32_t drops, const void *data, uint16_t size) 
{
	struct btsnoop_pkt pkt;
	uint64_t ts;
	DWORD written;

	if (!btsnoop || !tv)
		return false;

	if (btsnoop->max_size && btsnoop->max_size <=
		btsnoop->cur_size + size + BTSNOOP_PKT_SIZE)
		if (!btsnoop_rotate(btsnoop))
			return false;

	ts = (tv->tv_sec - 946684800ll) * 1000000ll + tv->tv_usec;

	pkt.size = htobe32(size);
	pkt.len = htobe32(size);
	pkt.flags = htobe32(flags);
	pkt.drops = htobe32(drops);
	pkt.ts = htobe64(ts + 0x00E03AB44A676000ll);

	if (FALSE == WriteFile(btsnoop->fd, &pkt, BTSNOOP_PKT_SIZE, &written, NULL)) {
		return false;
	}

	btsnoop->cur_size += BTSNOOP_PKT_SIZE;

	if (data && size > 0) {
		if (FALSE == WriteFile(btsnoop->fd, data, size, &written, NULL)) {
			return false;
		}
	}

	btsnoop->cur_size += size;

	return true;
}

static uint32_t get_flags_from_opcode(uint16_t opcode)
{
	switch (opcode) {
	case BTSNOOP_OPCODE_NEW_INDEX:
	case BTSNOOP_OPCODE_DEL_INDEX:
		break;
	case BTSNOOP_OPCODE_COMMAND_PKT:
		return 0x02;
	case BTSNOOP_OPCODE_EVENT_PKT:
		return 0x03;
	case BTSNOOP_OPCODE_ACL_TX_PKT:
		return 0x00;
	case BTSNOOP_OPCODE_ACL_RX_PKT:
		return 0x01;
	case BTSNOOP_OPCODE_SCO_TX_PKT:
	case BTSNOOP_OPCODE_SCO_RX_PKT:
		break;
	case BTSNOOP_OPCODE_ISO_TX_PKT:
	case BTSNOOP_OPCODE_ISO_RX_PKT:
		break;
	case BTSNOOP_OPCODE_OPEN_INDEX:
	case BTSNOOP_OPCODE_CLOSE_INDEX:
		break;
	}

	return 0xff;
}

bool btsnoop_write_hci(struct btsnoop *btsnoop, struct timeval *tv,
	uint16_t index, uint16_t opcode, uint32_t drops,
	const void *data, uint16_t size)
{
	uint32_t flags;

	if (!btsnoop)
		return false;

	switch (btsnoop->format) {
	case BTSNOOP_FORMAT_HCI:
		if (btsnoop->index == 0xffff)
			btsnoop->index = index;

		if (index != btsnoop->index)
			return false;

		flags = get_flags_from_opcode(opcode);
		if (flags == 0xff)
			return false;
		break;

	case BTSNOOP_FORMAT_MONITOR:
		flags = ((uint32_t)index << 16) | opcode;
		break;

	default:
		return false;
	}

	return btsnoop_write(btsnoop, tv, flags, drops, data, size);
}

bool btsnoop_write_phy(struct btsnoop *btsnoop, struct timeval *tv,
	uint16_t frequency, const void *data, uint16_t size)
{
	uint32_t flags;

	if (!btsnoop)
		return false;

	switch (btsnoop->format) {
	case BTSNOOP_FORMAT_SIMULATOR:
		flags = (1 << 16) | frequency;
		break;

	default:
		return false;
	}

	return btsnoop_write(btsnoop, tv, flags, 0, data, size);
}

bool btsnoop_read_hci(struct btsnoop *btsnoop, struct timeval *tv,
	uint16_t *index, uint16_t *opcode,
	void *data, uint16_t *size)
{
	return false;
}

bool btsnoop_read_phy(struct btsnoop *btsnoop, struct timeval *tv,
	uint16_t *frequency, void *data, uint16_t *size)
{
	return false;
}