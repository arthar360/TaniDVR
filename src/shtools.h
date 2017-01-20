/* shtools.h */

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

#ifndef SHTOOLS_H
#define SHTOOLS_H

#include <signal.h>
#include <stdint.h>

typedef enum {
	SHT_BASE_PROC,
	SHT_BUFFER_PROC,
	SHT_DVRCTL_PROC
} sht_context_t;

typedef enum {
	SHT_OP_BLOCK,
	SHT_OP_UNBLOCK
} sht_sigmgr_op_t;

#define SHT_F_SIGBLOCK_SIGSTD	(1<<0)
#define SHT_F_SIGBLOCK_SIGFATAL	(1<<1)

/* READ-ONLY */
extern volatile sig_atomic_t sht_fl_sigchld;
extern volatile sig_atomic_t sht_fl_sigpipe;
extern volatile sig_atomic_t sht_fl_terminate_nicely;

extern void sht_init (sht_context_t ctx);
extern void sht_signalblock_mgr (sht_sigmgr_op_t op, uint32_t sf);

#endif

