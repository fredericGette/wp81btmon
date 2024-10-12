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

/*
Called by the mainloop
Write HCI message to the btsnoop file when this file exists.
*/
static void data_callback(HANDLE fd, uint32_t events, void *user_data)
{
	struct control_data *data = user_data;
	unsigned char control[64];
	struct mgmt_hdr hdr;
	struct msghdr msg;
	struct iovec iov[2];

	if (events & (EPOLLERR | EPOLLHUP)) {
		mainloop_remove_fd(data->fd);
		return;
	}

	iov[0].iov_base = &hdr;
	iov[0].iov_len = MGMT_HDR_SIZE;
	iov[1].iov_base = data->buf;
	iov[1].iov_len = sizeof(data->buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	while (1) {
		struct cmsghdr *cmsg;
		struct timeval *tv = NULL;
		struct timeval ctv;
		struct ucred *cred = NULL;
		struct ucred ccred;
		uint16_t opcode, index, pktlen;
		ssize_t len;

		len = recvmsg(data->fd, &msg, MSG_DONTWAIT);
		if (len < 0)
			break;

		if (len < MGMT_HDR_SIZE)
			break;

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
					cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level != SOL_SOCKET)
				continue;

			if (cmsg->cmsg_type == SCM_TIMESTAMP) {
				memcpy(&ctv, CMSG_DATA(cmsg), sizeof(ctv));
				tv = &ctv;
			}

			if (cmsg->cmsg_type == SCM_CREDENTIALS) {
				memcpy(&ccred, CMSG_DATA(cmsg), sizeof(ccred));
				cred = &ccred;
			}
		}

		opcode = le16_to_cpu(hdr.opcode);
		index  = le16_to_cpu(hdr.index);
		pktlen = le16_to_cpu(hdr.len);

		switch (data->channel) {
		case HCI_CHANNEL_CONTROL:
			packet_control(tv, cred, index, opcode,
							data->buf, pktlen);
			break;
		case HCI_CHANNEL_MONITOR:
			btsnoop_write_hci(btsnoop_file, tv, index, opcode, 0,
							data->buf, pktlen);
			ellisys_inject_hci(tv, index, opcode,
							data->buf, pktlen);
			packet_monitor(tv, cred, index, opcode,
							data->buf, pktlen);
			break;
		}
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