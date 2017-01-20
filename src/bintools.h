/* bintools.h */

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

#ifndef BINTOOLS_H
#define BINTOOLS_H

#include <stdint.h>

/*
   MSB-ordered memory from/to native var.
   memx: uint8_t *memx / uint8_t memx[]
   varx: uint16_t / uint32_t / uint64_t

   memx MAY BE UNALIGNED.
*/

/* write unsigned 64/32/16-bit integer. usage:
   BT_NV2MM_U32(my_uint32,mybuff_p); */
#define BT_NV2MM_U64(memx,varx) \
	*(memx) = (((varx) >> 56) & 0xff); \
	*((memx) + 1) = (((varx) >> 48) & 0xff); \
	*((memx) + 2) = (((varx) >> 40) & 0xff); \
	*((memx) + 3) = (((varx) >> 32) & 0xff); \
	*((memx) + 4) = (((varx) >> 24) & 0xff); \
	*((memx) + 5) = (((varx) >> 16) & 0xff); \
	*((memx) + 6) = (((varx) >> 8) & 0xff); \
	*((memx) + 7) = ((varx) & 0xff);
#define BT_NV2MM_U32(memx,varx) \
	*(memx) = (((varx) >> 24) & 0xff); \
	*((memx) + 1) = (((varx) >> 16) & 0xff); \
	*((memx) + 2) = (((varx) >> 8) & 0xff); \
	*((memx) + 3) = ((varx) & 0xff);
#define BT_NV2MM_U16(memx,varx) \
	*(memx) = (((varx) >> 8) & 0xff); \
	*((memx) + 1) = ((varx) & 0xff);

/*
   LSB-ordered memory from/to native var.
   memx: uint8_t *memx / uint8_t memx[]
   varx: uint16_t / uint32_t / uint64_t

   memx MAY BE UNALIGNED.
*/

/* get unsigned 64/32/16-bit integer. usage:
   my_uint32 = BT_LM2NV_U32(mybuff_p); */
#define BT_LM2NV_U64(memx) \
	(*(memx) | \
	(*((memx) + 1) << 8) | \
	(*((memx) + 2) << 16) | \
	(*((memx) + 3) << 24) | \
	(*((memx) + 4) << 32) | \
	(*((memx) + 5) << 40) | \
	(*((memx) + 6) << 48) | \
	(*((memx) + 7) << 56))
#define BT_LM2NV_U32(memx) \
	(*(memx) | \
	(*((memx) + 1) << 8) | \
	(*((memx) + 2) << 16) | \
	(*((memx) + 3) << 24))
#define BT_LM2NV_U16(memx) \
	(*(memx) | \
	(*((memx) + 1) << 8))

/* write unsigned 64/32/16-bit integer. usage:
   BT_NV2LM_U32(my_uint32,mybuff_p); */
#define BT_NV2LM_U64(memx,varx) \
	*(memx) = ((varx) & 0xff); \
	*((memx) + 1) = (((varx) >> 8) & 0xff); \
	*((memx) + 2) = (((varx) >> 16) & 0xff); \
	*((memx) + 3) = (((varx) >> 24) & 0xff); \
	*((memx) + 4) = (((varx) >> 32) & 0xff); \
	*((memx) + 5) = (((varx) >> 40) & 0xff); \
	*((memx) + 6) = (((varx) >> 48) & 0xff); \
	*((memx) + 7) = (((varx) >> 56) & 0xff);
#define BT_NV2LM_U32(memx,varx) \
	*(memx) = ((varx) & 0xff); \
	*((memx) + 1) = (((varx) >> 8) & 0xff); \
	*((memx) + 2) = (((varx) >> 16) & 0xff); \
	*((memx) + 3) = (((varx) >> 24) & 0xff);
#define BT_NV2LM_U16(memx,varx) \
	*(memx) = ((varx) & 0xff); \
	*((memx) + 1) = (((varx) >>  8) & 0xff);




/*
   operations for LSB packed IDs in memory.
   memx: uint8_t *
   i0x..iNx: char (may be 8/16 bits, but uses lesser 8 bits only)

   memx MAY BE UNALIGNED.
*/

/* compares whether "string" ID is == to given 32 bits in memory
   (!=0, equal ; ==0, different).
   usage:
   if (BT_IDeqLM('w','o','r','d',mybuff_p) != 0) printf ("equal\n"); */
#define BT_IDeqLM_32(i0x,i1x,i2x,i3x,memx) \
	(( \
		(((uint8_t) (i0x) ^ *(memx)) | \
		((uint8_t) (i1x) ^ *(memx + 1)) | \
		((uint8_t) (i2x) ^ *(memx + 2)) | \
		((uint8_t) (i3x) ^ *(memx + 3))) \
	) ? 0 : 1)

#endif

