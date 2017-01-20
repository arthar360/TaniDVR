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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "mptools.h"

/* mptools_pipedfork must be preallocated manually */
/* returns: to parent, child PID. to child, 0. error -1 (fork()-alike) */
pid_t mptools_create_pipedfork (mptools_pipedfork_t *mptools_pipedfork)
{
	int tpp1[2]; /* [0]=read, [1]=write */
	int tpp2[2];

	mptools_pipedfork->wait_status = 0;

	if (pipe (tpp1) == 0) {
		if (pipe (tpp2) == 0) {
			if ((mptools_pipedfork->child_pid = fork ()) != -1) {
				if (mptools_pipedfork->child_pid != 0) {
					/* parent */
					close (tpp1[1]);
					close (tpp2[0]);
					mptools_pipedfork->fd_read = tpp1[0];
					mptools_pipedfork->fd_write = tpp2[1];

					return mptools_pipedfork->child_pid;
				} else {
					/* child */
					close (tpp1[0]);
					close (tpp2[1]);
					mptools_pipedfork->fd_read = tpp2[0];
					mptools_pipedfork->fd_write = tpp1[1];

					return (pid_t) 0;
				}
			}
			/* fork() error */
			close (tpp2[0]);
			close (tpp2[1]);
		}
		/* pipe2 failure */
		close (tpp1[0]);
		close (tpp1[1]);
	}
	/* pipe1 failure */

	return (pid_t) -1;	/* error at some point */
}

/* ret: true=ok, false=dead child (mptools_destroy_pipedfork_child() must be called still) */
bool mptools_check_pipedfork_child (mptools_pipedfork_t *mptools_pipedfork)
{
	return (waitpid (mptools_pipedfork->child_pid, &(mptools_pipedfork->wait_status), WNOHANG) == 0) ? true : false;
}

/* closes pipedfork from child's side (invoked by child itself) */
void mptools_child_close_pipedfork (mptools_pipedfork_t *mptools_pipedfork)
{
	close (mptools_pipedfork->fd_read);
	close (mptools_pipedfork->fd_write);
}

/* destroy child, brute force */
/* to be invoked from parent process */
void mptools_destroy_pipedfork (mptools_pipedfork_t *mptools_pipedfork)
{
	close (mptools_pipedfork->fd_read);
	close (mptools_pipedfork->fd_write);
	kill (mptools_pipedfork->child_pid, SIGTERM);
	waitpid (mptools_pipedfork->child_pid, &(mptools_pipedfork->wait_status), 0);

	/* TODO: implement flag to set mptools_pipedfork_t as inactive */

	return;
}


