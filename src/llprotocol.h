/* low-level protocol routines */

/* TaniDVR
 * Copyright (c) 2011-2015 Daniel Mealha Cabrita
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAS_LLPROTOCOL_H
#define HAS_LLPROTOCOL_H

#include <stdint.h>
#include <inttypes.h>
#include "network.h"

#define LLP_HEADER_SIZE 32
#define LLP_HD_VER_IHL 0x60
#define LLP_HD_NOP_REQ 0xa1
#define LLP_HD_NOP_REPLY 0xb1
#define llp_hd_cmd raw[0]
#define llp_hd_size_version raw[3]

typedef struct {
	uint8_t raw[LLP_HEADER_SIZE];
	uint32_t extlen;	/* mirrors/overrides raw[4]-raw[7] */
} t_ll_header;

typedef struct {
	t_net_connection net_connection;	/* PRIVATE */
} t_llp_connection;

extern int llp_open (t_llp_connection *llp_connection, const char *hostname, unsigned short int port, unsigned int timeout_us);
extern int llp_get_header (t_llp_connection *llp_connection, t_ll_header *ll_header);
extern int llp_get_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header);
extern int llp_get_extdata_sbuff (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t *buf, uint32_t buflen);
extern int llp_get_discard_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header);
extern void llp_init_header (t_ll_header *ll_header);
extern int llp_send_header (t_llp_connection *llp_connection, t_ll_header *ll_header);
extern int llp_send_nop (t_llp_connection *llp_connection);
extern int llp_send_extdata (t_llp_connection *llp_connection, uint8_t *payload, uint32_t len);
extern void llp_close (t_llp_connection *llp_connection);
extern int llp_check_incoming_data (t_llp_connection *llp_connection);
extern int llp_wait_for_incoming_data (t_llp_connection **llp_connection, int n_llp_connections, int wait_timeout);

#endif

