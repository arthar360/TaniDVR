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


#ifndef HAS_FILETOOLS_H
#define HAS_FILETOOLS_H

#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>

typedef struct {
        FILE *fd;
	bool fd_close; /* close on exit. false if stdout */
} t_inoutfile;

#define t_outfile t_inoutfile
#define t_infile t_inoutfile

extern t_outfile *outfile_open (const char *given_filename);
extern int outfile_write (t_outfile *outfile, uint8_t *data_p, size_t data_len);
extern void outfile_close (t_outfile *outfile);

extern t_infile *infile_open (const char *given_filename);
extern int infile_read (t_infile *infile, uint8_t *data_p, size_t buf_len);
extern void infile_close (t_infile *infile);



#endif

