/* dvrcontrol.h */

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

#ifndef DVRCONTROL_H
#define DVRCONTROL_H

#include <stdbool.h>
#include "hlprotocol.h"
#include "mptools.h"
#include "dvrcontrol.h"
#include "mctools.h"

/* to be provided when requesting a DVR connection */
typedef struct {
	const char *hostname;
	unsigned short int port;
	const char *user;
	const char *passwd;

	/* while streaming video, only */
	int channel;
	int sub_channel;
	bool ntsc_exact_60hz;
	tsproc_t tsproc;

	unsigned int keep_alive_us;	/* 0=disabled */
	unsigned int timeout_us;	/* 0=disabled */
	unsigned int net_protocol_dialect;	/* DVR protocol dialect to use */
} dvrcontrol_t;

extern int open_session (t_hlp_connection *conn_control, t_devinfo *devinfo, dvrcontrol_t *dvrctl);
extern int close_session (t_hlp_connection *conn_control);
extern int stream_media_dvr_to_pipe (mptools_pipedfork_t *ppfk, dvrcontrol_t *dvrctl);
extern int stream_media_dvr_to_file (dvrcontrol_t *dvrctl, int media_container, const char *filename);

#endif


