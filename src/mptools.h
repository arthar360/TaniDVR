/* mptools.h */
/* fork/IPC auxiliary rotines */

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

#ifndef MPTOOLS_H
#define MPTOOLS_H

#include <unistd.h>
#include <stdbool.h>

typedef struct {
	/* these are attributed accordingly depending whether
	   the process is a parent or a child.
	   the values are the appropriate copies from pipe_in/pipe_out */
	int fd_read;
	int fd_write;
	pid_t child_pid;	/* set to 0 at child process' side */
	int wait_status;	/* PRIVATE (used by parent) */
} mptools_pipedfork_t;

extern pid_t mptools_create_pipedfork (mptools_pipedfork_t *mptools_pipedfork);
extern bool mptools_check_pipedfork_child (mptools_pipedfork_t *mptools_pipedfork);
extern void mptools_child_close_pipedfork (mptools_pipedfork_t *mptools_pipedfork);
extern void mptools_destroy_pipedfork (mptools_pipedfork_t *mptools_pipedfork);


#endif


