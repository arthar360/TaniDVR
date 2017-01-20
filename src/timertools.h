/* timertools.h */

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

#ifndef TIMERTOOLS_H
#define TIMERTOOLS_H

#include <sys/time.h>

typedef struct {
	struct timeval last;
	struct timeval current;
} spenttime_t;

extern int spenttime_set (spenttime_t *spt);
extern unsigned int spenttime_get (spenttime_t *spt);

#endif

