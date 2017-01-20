/* bufftools.c */

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

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <errno.h>

#include "bufftools.h"

/* BEFORE calling this, initialize btfifo_t parameters */
/* returns: ==0, ok btfifo properly initialized; !=0, error */
int btfifo_create (btfifo_t *btfifo)
{
	if ((btfifo->b = malloc (btfifo->bsize)) == NULL)
		return 1;

	/* fd in */
	btfifo->p[0].fd = -1;
	btfifo->p[1].events = POLLIN | POLLPRI;
	/* fd out */
	btfifo->p[1].fd = -1;
	btfifo->p[1].events = POLLOUT;

	btfifo->dlen = 0;
	btfifo->dpos = 0;

	return 0;
}

/* BEFORE calling this, fd MUST be open and set to NON-BLOCKING */
/* returns: total of bytes readen from fd (0 if nothing to read or fifo full) ;
   or <0 if error (no data written):
   - if <0 && >-100, may attempt again later.
   - if <= -100, fatal error. */
/* to improve performance, call this only when:
   - fd has data to read */
ssize_t btfifo_fd_to_fifo (btfifo_t *btfifo, int fd)
{
	int pollret;
	int f_regions;	/* total number of free, contiguous, buffer regions (1 or 2) */
	size_t f_tot;	/* total free buffer space, contiguous or not */
	ssize_t	retread;
	size_t f_len;	/* free length of current region */
	uint8_t *wpos;

	btfifo->p[0].fd = fd;

	/* preliminary fd test.
	   RECOMENDATION: main code should perform custom poll()s by itself. */
	if (! (btfifo->flags & BTFIFO_F_ASSUME_FD_READY)) {
		if ((pollret = poll (&(btfifo->p[0]), 1, 0)) == -1)
			return -103;
		if ((btfifo->p[0].revents) & (POLLERR | POLLHUP | POLLNVAL))
			return -1201;	/* fd collapsed */
	}

	if ((f_tot = btfifo->bsize - btfifo->dlen) == 0)
		return -2;	/* buffer full */

	f_regions = ((btfifo->dpos != 0) && ((btfifo->dpos + btfifo->dlen) < btfifo->bsize)) ? 2 : 1;

	if (f_regions == 2) {
		f_len = btfifo->bsize - (btfifo->dpos + btfifo->dlen);
		wpos = btfifo->b + btfifo->dpos + btfifo->dlen;
		if ((retread = read (fd, wpos, f_len)) == -1) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
				return -1;	/* nothing to read */
			return -1301;	/* fd collapsed */
		}
		if (retread == 0)
			return 0;	/* got nothing */
		btfifo->dlen += retread;

		if (retread != f_len)
			return retread;	/* not enough to fill the 1st region */
	}

	/* f_regions == 1 OR writing into the 2nd region */
	f_len = btfifo->bsize - btfifo->dlen;
	wpos = (btfifo->dpos == 0) ? (btfifo->b + btfifo->dlen) : (btfifo->b + (btfifo->dpos - f_len));
	if ((retread = read (fd, wpos, f_len)) == -1) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			return -1;	/* nothing to read */
		return -1401;	/* fd collapsed */
	}
	if (retread == 0)
		return 0;	/* got nothing */
	btfifo->dlen += retread;
	return retread;
}

/* BEFORE calling this, fd MUST be open and set to NON-BLOCKING */
/* returns: total of bytes written to fd ;
   or <0 if error (no data written):
   - if <0 && >-100, may attempt again later.
   - if <= -100, fatal error. */
/* to improve performance, call this only when:
   - fd allows non-blocking/atomic writes
   - FIFO is not empty */
ssize_t btfifo_fifo_to_fd (btfifo_t *btfifo, int fd)
{
	int pollret;
	int u_regions;	/* total number of used, contiguous, buffer regions (1 or 2) */
	size_t u_tot = btfifo->dlen;
	ssize_t retwrite;
	size_t u_len;	/* used length of current region */
	uint8_t *rpos;

	/* preliminary fd test.
	   RECOMENDATION: main code should perform custom poll()s by itself. */
	if (! (btfifo->flags & BTFIFO_F_ASSUME_FD_READY)) {
		btfifo->p[1].fd = fd;
		if ((pollret = poll (&(btfifo->p[1]), 1, 0)) == -1)
			return -103;
		if ((btfifo->p[1].revents) & (POLLERR | POLLHUP | POLLNVAL))
			return -101;	/* fd collapsed */
	}

	if (btfifo->dlen == 0)
		return 0;	/* empty buffer */
	if ((btfifo->flags & BTFIFO_F_USE_WMIN) && (btfifo->wmin > u_tot))
		return 0;	/* too little data to bother */

	u_regions = ((btfifo->dpos != 0) && ((btfifo->dpos + u_tot) > btfifo->bsize)) ? 2 : 1;
	if (u_regions == 2) {
		u_len = btfifo->bsize - btfifo->dpos;
		rpos = btfifo->b + btfifo->dpos;
		if ((retwrite = write (fd, rpos, u_len)) == -1) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
				return -1;	/* cannot write now */
			return -101;	/* fd collapsed */
		}
		if (retwrite == 0)
			return 0;	/* sent nothing */

		btfifo->dlen -= retwrite;
		if (retwrite != u_len) {
			btfifo->dpos += retwrite;
			return retwrite;
		}
		btfifo->dpos = 0;
	}

	/* u_regions == 1 OR reading data from the 2nd region */
	u_len = btfifo->dlen;
	rpos = btfifo->b + btfifo->dpos;
	if ((retwrite = write (fd, rpos, u_len)) == -1) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			return -1;	/* cannot write now */
		return -101;	/* fd collapsed */
	}
	if (retwrite == 0)
		return 0;	/* sent nothing */

	/* if FIFO is empty, set offset back to beginning for
	   better performance (by avoiding unnecessary buffer cycling) */
	if (btfifo->dlen == 0)
		btfifo->dpos = 0;

	btfifo->dlen -= retwrite;
	btfifo->dpos += retwrite;
	if (btfifo->dpos == btfifo->bsize)
		btfifo->dpos = 0;

	return retwrite;
}

/* get how many bytes are free in FIFO */
size_t btfifo_get_free_len (btfifo_t *btfifo)
{
	return btfifo->bsize - btfifo->dlen;
}


/* get how many bytes are stored in FIFO */
size_t btfifo_get_stored_len (btfifo_t *btfifo)
{
	return btfifo->dlen;
}

/* BEFORE calling this,
   in order to force a FIFO flush and avoid data loss:
   - set write fd to BLOCKING
   - disable BTFIFO_F_USE_WMIN (if enabled before)
   - call btfifo_fifo_to_fd()  */
void btfifo_destroy (btfifo_t *btfifo)
{
	free (btfifo->b);
}


