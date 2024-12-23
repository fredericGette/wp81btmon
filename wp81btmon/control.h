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

bool control_writer(const char *path);
void control_reader(const char *path, bool pager);
void control_server(const char *path);
int control_tty(const char *path, unsigned int speed);
int control_rtt(char *jlink, char *rtt);
int control_tracing(void);
void control_disable_decoding(void);
void control_filter_index(uint16_t index);

void control_message(uint16_t opcode, const void *data, uint16_t size);

void *control_get_tracing(void);

void control_block_bthx(void);
void control_allow_bthx(void);

bool control_read_events(void);
void control_init_read_events(void);
void control_cleanup_read_events(void);

bool control_send_commands(void);
void control_init_send_commands(void);
void control_cleanup_send_commands(void);

bool control_read_data(void);
void control_init_read_data(void);
void control_cleanup_read_data(void);