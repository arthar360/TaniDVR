/* high-level protocol routines */

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

#ifndef HAS_HLPROTOCOL_H
#define HAS_HLPROTOCOL_H

#include <stdint.h>
#include <inttypes.h>
#include "llprotocol.h"
#include "devinfo.h"

#define MAX_USER_PASSWD_LEN 250

typedef struct {
	t_llp_connection llp_connection;
	uint32_t unique_login_symbol; /* automatically provided upon hlp_login() call */
} t_hlp_connection;

extern int hlp_open (t_hlp_connection *hlp_connection, const char *hostname, unsigned short int port, unsigned int timeout_us);
extern void hlp_close (t_hlp_connection *hlp_connection);
extern int hlp_get_header (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t expected_cmd);
extern int hlp_get_header_and_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t expected_cmd, int issue_nop, uint8_t *buf, uint32_t buflen, int *extdata_len);
extern int hlp_login (t_hlp_connection *hlp_connection, t_devinfo *devinfo, const char *user, const char *passwd);
extern int hlp_logout (t_hlp_connection *hlp_connection);
int hlp_media_data_request (t_hlp_connection *hlp_connection_ctrl, t_hlp_connection *hlp_connection_data, int channel, int sub_channel, uint8_t *dest_data_p, size_t max_len, size_t *dest_data_len);
extern int hlp_collect_media_data (t_hlp_connection *hlp_connection_data, int channel, uint8_t *dest_data_p, size_t max_len, size_t *dest_data_len);
extern int hlp_connection_relationship (t_hlp_connection *hlp_connection_orig, t_hlp_connection *hlp_connection_new, uint8_t reqtype, uint8_t reqchnumber);
extern int hlp_send_extension_string (t_hlp_connection *hlp_connection, const char *extstr);
extern int hlp_get_work_alarm_status (t_hlp_connection *hlp_connection);
extern int hlp_keep_alive (t_hlp_connection *hlp_connection);
#ifdef DEBUG
extern void hlp_debug_dump_header (t_ll_header *ll_header, int nonzero);
extern void hlp_debug_dump_extdata (uint8_t *extdata_p, int extdata_len);
#endif
extern int hlp_get_system_information (t_hlp_connection *hlp_connection, t_devinfo *devinfo, uint8_t infotype, uint8_t bitstream);
extern int hlp_get_system_information_multiple (t_hlp_connection *hlp_connection, t_devinfo *devinfo, uint8_t first_infotype, int total_infotype);
extern int hlp_get_media_capability (t_hlp_connection *hlp_connection, uint8_t infotype);
extern int hlp_get_config_parameter (t_hlp_connection *hlp_connection, uint8_t infotype);
extern int hlp_set_alarm (t_hlp_connection *hlp_connection, uint8_t alarm_type);
extern int hlp_get_channel_names (t_hlp_connection *hlp_connection, char **outb);
extern int hlp_discard_incoming_data (t_hlp_connection *hlp_connection);
extern int hlp_check_incoming_data (t_hlp_connection *hlp_connection);
extern int hlp_wait_for_incoming_data (t_hlp_connection **hlp_connection, int n_hlp_connections, int wait_timeout);

#endif

