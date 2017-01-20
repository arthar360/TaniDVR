/* file manipulation routines */

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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "filetools.h"

/* OUTPUT file */

/* opens a file or, if given_filename is empty, uses stdout */
t_outfile *outfile_open (const char *given_filename)
{
	t_outfile *outfile;

	if ((outfile = malloc (sizeof (t_outfile))) == NULL)
		return NULL;

	outfile->fd_close = true;

	if (*given_filename == '\0') {
		outfile->fd = stdout;
		outfile->fd_close = false;
	} else if ((outfile->fd = fopen (given_filename, "w+b")) == NULL) {
		free (outfile);
		return NULL;
	}

	return outfile;
}

/* returns ==0 ok, !=0 error */
int outfile_write (t_outfile *outfile, uint8_t *data_p, size_t data_len)
{
	if (fwrite (data_p, 1, data_len, outfile->fd) < data_len)
		return -1;
	return (fflush (outfile->fd));
}

void outfile_close (t_outfile *outfile)
{
	if (outfile->fd_close == true)
		fclose (outfile->fd);
}



/* INPUT file */

/* opens a file or, if given_filename is empty, uses stdin */
t_infile *infile_open (const char *given_filename)
{
	t_infile *infile;

	if ((infile = malloc (sizeof (t_infile))) == NULL)
		return NULL;

	infile->fd_close = true;

	if (*given_filename == '\0') {
		infile->fd = stdin;
		infile->fd_close = false;
	} else if ((infile->fd = fopen (given_filename, "rb")) == NULL) {
		free (infile);
		return NULL;
	}

	return infile;
}

/* returns as fread does */
int infile_read (t_infile *infile, uint8_t *data_p, size_t buf_len)
{
	return (fread (data_p, 1, buf_len, infile->fd));
}

void infile_close (t_infile *infile)
{
	if (infile->fd_close == true)
		fclose (infile->fd);
}

