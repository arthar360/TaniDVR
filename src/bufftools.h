/* bufftools.h */

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
#include <stddef.h>
#include <poll.h>
#include <limits.h>

#ifndef BUFFTOOLS_H
#define BUFFTOOLS_H

#define BTFIFO_BUFF_MAXSIZE (SSIZE_MAX - 1)
#define BTFIFO_BUFF_MINSIZE 4096

#define BTFIFO_F_NONE			0
#define BTFIFO_F_USE_WMIN		(1<<0)
#define BTFIFO_F_ASSUME_FD_READY	(1<<1)

typedef struct {
	/* these must be initialized before calling btfifo_create() */
	size_t bsize;	/* buffer size, must be < BTFIFO_FIFO_MAXSIZE and >= BTFIFO_BUFF_MINSIZE */
	uint32_t flags;	/* if nothing special, use BTFIFO_F_NONE */

	/* this must be initialized if != BTFIFO_F_DEFAULT and depending on set flags */
	size_t wmin;	/* attempt write when FIFO has, at least, wmin bytes. requires BTFIFO_F_USE_WMIN */

	/* PRIVATE */
	uint8_t *b;     /* fifo data, circular FIFO */
	size_t dlen;	/* data length */
	size_t dpos;	/* offset to b; start of useful data */
	struct pollfd p[2];	/* [0] fd_in ; [1] fd_out */
} btfifo_t;

extern int btfifo_create (btfifo_t *btfifo);
extern ssize_t btfifo_fd_to_fifo (btfifo_t *btfifo, int fd);
extern ssize_t btfifo_fifo_to_fd (btfifo_t *btfifo, int fd);
extern size_t btfifo_get_free_len (btfifo_t *btfifo);
extern size_t btfifo_get_stored_len (btfifo_t *btfifo);
extern void btfifo_destroy (btfifo_t *btfifo);

#endif

