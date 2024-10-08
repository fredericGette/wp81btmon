#include "stdafx.h"

struct packet_conn_data *packet_get_conn_data(uint16_t handle)
{
	return NULL;
}

void packet_latency_add(struct packet_latency *latency, struct timeval *delta) {}

bool packet_has_filter(unsigned long filter)
{
	return false;
}
void packet_set_filter(unsigned long filter){}
void packet_add_filter(unsigned long filter){}
void packet_del_filter(unsigned long filter){}

void packet_set_priority(const char *priority){}
void packet_select_index(uint16_t index){}
void packet_set_fallback_manufacturer(uint16_t manufacturer){}
void packet_set_msft_evt_prefix(const uint8_t *prefix, uint8_t len){}

void packet_hexdump(const unsigned char *buf, uint16_t len){}
void packet_print_error(const char *label, uint8_t error){}
void packet_print_version(const char *label, uint8_t version,
	const char *sublabel, uint16_t subversion){}
void packet_print_company(const char *label, uint16_t company){}
void packet_print_addr(const char *label, const void *data, uint8_t type){}
void packet_print_handle(uint16_t handle){}
void packet_print_rssi(const char *label, int8_t rssi){}
void packet_print_ad(const void *data, uint8_t size){}
void packet_print_features_lmp(const uint8_t *features, uint8_t page){}
void packet_print_features_ll(const uint8_t *features){}
void packet_print_features_msft(const uint8_t *features){}
void packet_print_channel_map_lmp(const uint8_t *map){}
void packet_print_channel_map_ll(const uint8_t *map){}
void packet_print_io_capability(uint8_t capability){}
void packet_print_io_authentication(uint8_t authentication){}
void packet_print_codec_id(const char *label, uint8_t codec){}

void packet_control(struct timeval *tv, struct ucred *cred,
	uint16_t index, uint16_t opcode,
	const void *data, uint16_t size){}
void packet_monitor(struct timeval *tv, struct ucred *cred,
	uint16_t index, uint16_t opcode,
	const void *data, uint16_t size){}
void packet_simulator(struct timeval *tv, uint16_t frequency,
	const void *data, uint16_t size){}

void packet_new_index(struct timeval *tv, uint16_t index, const char *label,
	uint8_t type, uint8_t bus, const char *name){}
void packet_del_index(struct timeval *tv, uint16_t index, const char *label){}
void packet_open_index(struct timeval *tv, uint16_t index, const char *label){}
void packet_close_index(struct timeval *tv, uint16_t index, const char *label){}
void packet_index_info(struct timeval *tv, uint16_t index, const char *label,
	uint16_t manufacturer){}
void packet_vendor_diag(struct timeval *tv, uint16_t index,
	uint16_t manufacturer,
	const void *data, uint16_t size){}
void packet_system_note(struct timeval *tv, struct ucred *cred,
	uint16_t index, const void *message){}
void packet_user_logging(struct timeval *tv, struct ucred *cred,
	uint16_t index, uint8_t priority,
	const char *ident, const void *data,
	uint16_t size){}

void packet_hci_command(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}
void packet_hci_event(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}
void packet_hci_acldata(struct timeval *tv, struct ucred *cred, uint16_t index,
	bool in, const void *data, uint16_t size) {}
void packet_hci_scodata(struct timeval *tv, struct ucred *cred, uint16_t index,
	bool in, const void *data, uint16_t size) {}
void packet_hci_isodata(struct timeval *tv, struct ucred *cred, uint16_t index,
	bool in, const void *data, uint16_t size) {}

void packet_ctrl_open(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}
void packet_ctrl_close(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}
void packet_ctrl_command(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}
void packet_ctrl_event(struct timeval *tv, struct ucred *cred, uint16_t index,
	const void *data, uint16_t size) {}

void packet_todo(void) {}