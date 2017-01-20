/* network interfacing routines */

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

#ifndef HAS_NETWORK_H
#define HAS_NETWORK_H

#include <stdio.h>
#include <sys/time.h>

typedef struct {
	FILE *net_sockrfp;
	FILE *net_sockwfp;
	int net_sockfd;
	struct timeval timeout_rx; /* network recv timeout */
	struct timeval timeout_tx; /* network send timeout */
} t_net_connection;

extern int net_open (t_net_connection *net_connection, const char *hostname, unsigned short int port, unsigned int sock_timeout_us);
extern void net_close (t_net_connection *net_connection);

#endif

