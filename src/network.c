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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include "network.h"

typedef int SOCKET;

/* FIXME: that should go to autoconf */
#define USE_IPV6

#define MAX_SA_ENTRIES 10

int open_client_socket (const char *hostname, unsigned short int Port, struct timeval *timeout_rx, struct timeval *timeout_tx);

int net_open (t_net_connection *net_connection, const char *hostname, unsigned short int port, unsigned int sock_timeout_us)
{
	time_t to_s = sock_timeout_us / 1000000;
	time_t to_us = sock_timeout_us % 1000000;

	net_connection->timeout_rx.tv_sec = to_s;
	net_connection->timeout_rx.tv_usec = to_us;
	net_connection->timeout_tx.tv_sec = to_s;
	net_connection->timeout_tx.tv_usec = to_us;

	net_connection->net_sockfd = open_client_socket (hostname, port, &(net_connection->timeout_rx), &(net_connection->timeout_tx));
	if (net_connection->net_sockfd < 0)
		return (-1 * net_connection->net_sockfd);

	net_connection->net_sockrfp = fdopen (net_connection->net_sockfd, "r");
	net_connection->net_sockwfp = fdopen (net_connection->net_sockfd, "w");

	return 0;
}

void net_close (t_net_connection *net_connection)
{
	fclose (net_connection->net_sockrfp);
	fclose (net_connection->net_sockwfp);
}

/* if timeout_<tx|rx> != NULL, set timeouts accordingly */
int open_client_socket (const char *hostname, unsigned short int Port, struct timeval *timeout_rx, struct timeval *timeout_tx)
{
#ifdef USE_IPV6
    struct addrinfo hints;
    char Portstr[10];
    int gaierr;
    struct addrinfo* ai;
    struct addrinfo* ai2;
    struct addrinfo* aiv4;
    struct addrinfo* aiv6;
    struct sockaddr_in6 sa[MAX_SA_ENTRIES];
#else /* USE_IPV6 */
    struct hostent *he;
    struct sockaddr_in sa[MAX_SA_ENTRIES];
#endif /* USE_IPV6 */
    int sa_len, sock_family, sock_type, sock_protocol;
    int sockfd;
    int sa_entries = 0;

    memset( (void*) &sa, 0, sizeof(sa) );

#ifdef USE_IPV6
#define SIZEOF_SA sizeof(struct sockaddr_in6)
#else
#define SIZEOF_SA sizeof(struct sockaddr_in)
#endif

#ifdef USE_IPV6
    (void) memset( &hints, 0, sizeof(hints) );
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    (void) snprintf( Portstr, sizeof(Portstr), "%hu", Port );
    if ( (gaierr = getaddrinfo( hostname, Portstr, &hints, &ai )) != 0 ) {
	/* ERROR: unknown host */
        return (-1);
    }

    /* Find the first IPv4 and IPv6 entries. */
    aiv4 = NULL;
    aiv6 = NULL;
    for ( ai2 = ai; ai2 != NULL; ai2 = ai2->ai_next )
	{
	switch ( ai2->ai_family )
	    {
	    case AF_INET: 
	    if ( aiv4 == NULL )
		aiv4 = ai2;
	    break;
	    case AF_INET6:
	    if ( aiv6 == NULL )
		aiv6 = ai2;
	    break;
	    }
	}

    /* If there's an IPv4 address, use that, otherwise try IPv6. */
    if (aiv4 != NULL) {
	if (SIZEOF_SA < aiv4->ai_addrlen) {
		/* ERROR: sockaddr too small */
		return (-1);
	}
	sock_family = aiv4->ai_family;
	sock_type = aiv4->ai_socktype;
	sock_protocol = aiv4->ai_protocol;
	sa_len = aiv4->ai_addrlen;

	/* loops each returned IP address and fills the array */
	{	
		struct addrinfo* current_aiv4 = aiv4;
		
		(void) memmove (&sa[sa_entries++], current_aiv4->ai_addr, sa_len);
		while ((sa_entries < MAX_SA_ENTRIES) && (current_aiv4->ai_next != NULL)) {
			current_aiv4 = current_aiv4->ai_next;
			(void) memmove (&sa[sa_entries++], current_aiv4->ai_addr, sa_len);
		}
	}
	
	goto ok;
	}
    if ( aiv6 != NULL )
	{
	if ( SIZEOF_SA < aiv6->ai_addrlen )
	    {
                /* ERROR: sockaddr too small */
                return (-1);
	    }
	sock_family = aiv6->ai_family;
	sock_type = aiv6->ai_socktype;
	sock_protocol = aiv6->ai_protocol;
	sa_len = aiv6->ai_addrlen;

	/* loops each returned IP address and fills the array */
	{
		struct addrinfo* current_aiv6 = aiv6;

		(void) memmove (&sa[sa_entries++], current_aiv6->ai_addr, sa_len);
		while ((sa_entries < MAX_SA_ENTRIES) && (current_aiv6->ai_next != NULL)) {
			current_aiv6 = current_aiv6->ai_next;
			(void) memmove (&sa[sa_entries++], current_aiv6->ai_addr, sa_len);
		}
	}

	goto ok;
	}

    /* ERROR: unknown host */
    return (1);

    ok:
    freeaddrinfo( ai );

#else /* USE_IPV6 */

    he = gethostbyname( hostname );
    if ( he == NULL ) {
        /* ERROR: unknown host */
        return (-1);
    }
    sock_family = he->h_addrtype;
    sock_type = SOCK_STREAM;
    sock_protocol = 0;
    sa_len = SIZEOF_SA;

    /* loops each returned IP address and fills the array */
    while ((sa_entries < MAX_SA_ENTRIES) && (he->h_addr_list[sa_entries] != NULL)) {
	(void) memmove (&sa[sa_entries].sin_addr, he->h_addr_list[sa_entries], sizeof (sa[sa_entries].sin_addr));
	sa[sa_entries].sin_family = he->h_addrtype;
	sa[sa_entries].sin_port = htons (Port);
	sa_entries++;
    }
    
#endif /* USE_IPV6 */

    sockfd = socket( sock_family, sock_type, sock_protocol );
    if ( sockfd < 0 ) {
        /* ERROR: internal error. could not create socket. */
        return (-1);
    }

    /* try each returned IP of that hostname */
    while (sa_entries--){
	if (connect (sockfd, (struct sockaddr*) &sa[sa_entries], sa_len) >= 0) {
		/* if timeouts are specified, set them */
		if (timeout_rx != NULL)
			setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, timeout_rx, sizeof (struct timeval));
		if (timeout_tx != NULL)
			setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, timeout_tx, sizeof (struct timeval));

		return sockfd;
	}
    }

    /* ERROR: connection refused */
    return (-1);
}

