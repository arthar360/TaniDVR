/* timertools.c */

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

#include <sys/time.h>
#include <stdlib.h>

#include "timertools.h"

/* updates time with current one, rolls previous time */
/* returns: 0,ok -1,error */
int spenttime_set (spenttime_t *spt)
{
	return gettimeofday (&(spt->last), NULL);
}

/* returns difference between last time and current in micro seconds */
unsigned int spenttime_get (spenttime_t *spt)
{
	gettimeofday (&(spt->current), NULL);
	return (unsigned int) ((((unsigned long long int) spt->current.tv_sec * 1000000) + spt->current.tv_usec) - \
		(((unsigned long long int) spt->last.tv_sec * 1000000) + spt->last.tv_usec));
}


