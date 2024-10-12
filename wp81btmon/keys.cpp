#include "stdafx.h"

void keys_setup(void)
{
}

void keys_cleanup(void)
{}

void keys_update_identity_key(const uint8_t key[16])
{}

void keys_update_identity_addr(const uint8_t addr[6], uint8_t addr_type)
{}

bool keys_resolve_identity(const uint8_t addr[6], uint8_t ident[6],
	uint8_t *ident_type)
{
	return false;
}

bool keys_add_identity(const uint8_t addr[6], uint8_t addr_type,
	const uint8_t key[16])
{
	return false;
}