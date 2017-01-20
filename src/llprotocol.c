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

#include <strings.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include "llprotocol.h"
#include "network.h"
#include "bintools.h"

int llp_open (t_llp_connection *llp_connection, const char *hostname, unsigned short int port, unsigned int timeout_us)
{
	return (net_open (&(llp_connection->net_connection), hostname, port, timeout_us));
}

int llp_get_header (t_llp_connection *llp_connection, t_ll_header *ll_header)
{
	llp_init_header (ll_header);	/* zero frame, just in case if partial fread() */

	if (fread (ll_header->raw, 1, LLP_HEADER_SIZE, llp_connection->net_connection.net_sockrfp) != LLP_HEADER_SIZE) {
		return 1;
	}

	ll_header->extlen = BT_LM2NV_U32((ll_header->raw) + 4);

	return 0;
}

/* TODO: implement this */
int llp_get_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header)
{
//	if (ll_header->extdata == NULL)
//		return 1;
//	if (fread (ll_header->extdata, 1, ll_header->extlen, llp_connection->net_connection.net_sockrfp) != ll_header->extlen)
//		return 1;

	return 0;
}

/* returns the length of data, or 0 if there's none, or a negative value (error). */
int llp_get_extdata_sbuff (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t *buf, uint32_t buflen)
{
	if (ll_header->extlen == 0)
		return 0;

	if (buflen < ll_header->extlen)
		return -2; /* not enough buffer */

	if (fread (buf, 1, ll_header->extlen, llp_connection->net_connection.net_sockrfp) != ll_header->extlen)
		return -1; /* read failed */

	return ll_header->extlen;
}



#define DISCARD_CHUNK_SIZE 65536
/* ll_header must contain a valid header */
/* returns: ==0 ok, !=0 error */
int llp_get_discard_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header)
{
	uint32_t rem_extlen = ll_header->extlen;
	uint8_t discard[DISCARD_CHUNK_SIZE];
	uint32_t to_read;

	if (rem_extlen == 0)
		return 0;

	while (rem_extlen != 0) {
		if (rem_extlen > DISCARD_CHUNK_SIZE) {
			to_read = DISCARD_CHUNK_SIZE;
		} else {
			to_read = rem_extlen;
		}

		if (fread (discard, 1, to_read, llp_connection->net_connection.net_sockrfp) != to_read)
			return 2;
		rem_extlen -= to_read;
	}

	return 0;
}

void llp_init_header (t_ll_header *ll_header)
{
	bzero (ll_header, sizeof (t_ll_header));
}

int llp_send_header (t_llp_connection *llp_connection, t_ll_header *ll_header)
{
	BT_NV2LM_U32((ll_header->raw) + 4,(ll_header->extlen))
	if (fwrite (ll_header->raw, 1, LLP_HEADER_SIZE, llp_connection->net_connection.net_sockwfp) != LLP_HEADER_SIZE)
		return 1;
	fflush (llp_connection->net_connection.net_sockwfp);

	return 0;
}

/* send an innocuous request.
   this is used to detect lack of response from a previous request,
   otherwise the waiting would hog fread() until timeout */
int llp_send_nop (t_llp_connection *llp_connection)
{
	t_ll_header ll_header;

	llp_init_header (&ll_header);
	ll_header.llp_hd_cmd = LLP_HD_NOP_REQ;

	return llp_send_header (llp_connection, &ll_header);
}

int llp_send_extdata (t_llp_connection *llp_connection, uint8_t *payload, uint32_t len)
{
	if (fwrite (payload, 1, len, llp_connection->net_connection.net_sockwfp) != len)
		return 1;
	fflush (llp_connection->net_connection.net_sockwfp);
	return 0;
}

void llp_close (t_llp_connection *llp_connection)
{
	net_close (&(llp_connection->net_connection));
}

/* returns >0 if there's pending data to be readen, ==0 otherwise, <0 if error */
int llp_check_incoming_data (t_llp_connection *llp_connection)
{
	struct pollfd sock_pollfd;

	sock_pollfd.fd = llp_connection->net_connection.net_sockfd;
	sock_pollfd.events = POLLIN;
	return (poll(&sock_pollfd, 1, 0));
}

/* wait for one or more connections to have incoming data, or until wait_timeout (ms) is reached.
   returns: >0 incoming data, ==0 no data, <0 error */
int llp_wait_for_incoming_data (t_llp_connection **llp_connection, int n_llp_connections, int wait_timeout)
{
	struct pollfd sock_pollfd[n_llp_connections];
	int i;

	for (i = 0; i < n_llp_connections; i++) {
		sock_pollfd[i].fd = llp_connection[i]->net_connection.net_sockfd;
		sock_pollfd[i].events = POLLIN;
	}
	return (poll(&sock_pollfd[0], n_llp_connections, wait_timeout));
}


