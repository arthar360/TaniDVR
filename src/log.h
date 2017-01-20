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

#ifndef HAS_LOG_H
#define HAS_LOG_H

typedef enum {
	LOGT_FATAL,
	LOGT_ERROR,
	LOGT_WARNING,
	LOGT_INFO,
	LOGT_DETAIL
} t_log_type;

#ifdef DEBUG
extern int DEBUG_LOG_PRINTF (char *fmt, ...);
extern int DEBUG_LOG_PRINTF_RAW (char *fmt, ...);
#else
#define DEBUG_LOG_PRINTF //
#endif

extern int log_printf (t_log_type log_type, char *fmt, ...);
extern void log_define_context (const char *ctxname);

#endif

