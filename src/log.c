/* logging routines */

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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"

#define LOG_CONTEXT_MAX_STRLEN 255

static char log_context_str[LOG_CONTEXT_MAX_STRLEN + 1];
static const char *log_context_str_p = NULL;

#ifdef DEBUG
int DEBUG_LOG_PRINTF (char *fmt, ...)
{
        va_list ap;
        int i;

        va_start(ap, fmt);	/* macro */
        fprintf (stderr, "DEBUG: ");
        i = vfprintf (stderr, fmt, ap);
        va_end(ap);		/* macro */
        return i;
}

int DEBUG_LOG_PRINTF_RAW (char *fmt, ...)
{
        va_list ap;
        int i;

        va_start(ap, fmt);	/* macro */
        i = vfprintf (stderr, fmt, ap);
        va_end(ap);		/* macro */
        return i;
}
#endif

int log_printf (t_log_type log_type, char *fmt, ...)
{
        va_list ap;
        int i;

	if (log_context_str_p != NULL) {
		switch (log_type) {
		case LOGT_FATAL:	fprintf (stderr, "FATAL ERROR (%s): ", log_context_str_p);	break;
		case LOGT_ERROR:	fprintf (stderr, "ERROR (%s): ", log_context_str_p);	break;
		case LOGT_WARNING:	fprintf (stderr, "WARNING (%s): ", log_context_str_p);	break;
		case LOGT_INFO:		fprintf (stderr, "INFO (%s): ", log_context_str_p);	break;
		case LOGT_DETAIL:	fprintf (stderr, "DETAIL (%s): ", log_context_str_p);	break;
		}
	} else {
		switch (log_type) {
		case LOGT_FATAL:	fprintf (stderr, "FATAL ERROR: ");	break;
		case LOGT_ERROR:	fprintf (stderr, "ERROR: ");	break;
		case LOGT_WARNING:	fprintf (stderr, "WARNING: ");	break;
		case LOGT_INFO:		fprintf (stderr, "INFO: ");	break;
		case LOGT_DETAIL:	fprintf (stderr, "DETAIL: ");	break;
		}
	}

        va_start(ap, fmt);	/* macro */
        i = vfprintf (stderr, fmt, ap);
        va_end(ap);		/* macro */
        return i;
}

void log_define_context (const char *ctxname)
{
	if (ctxname == NULL) {
		log_context_str_p = NULL;
		return;
	}
	strncpy (log_context_str, ctxname, LOG_CONTEXT_MAX_STRLEN + 1);
	log_context_str[LOG_CONTEXT_MAX_STRLEN] = '\0';
	log_context_str_p = log_context_str;
}

