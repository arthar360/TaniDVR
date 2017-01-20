/* media container manipulation routines */

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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mctools.h"
#include "log.h"
#include "bintools.h"
#include "config.h"	/* autotools-generated */

#define WHOLE_MAIN_HEADER_LOAD 416 + 4

/* offsets of interest in mkv_head[] */
#define MKV_HOFF_CodecStr	(0x153)
#define MKV_HOFF_PixelWidth	(0x16e + 2)
#define MKV_HOFF_PixelHeight	(0x172 + 2)
#define MKV_HOFF_DisplayWidth	(0x176 + 3)
#define MKV_HOFF_DisplayHeight	(0x17b + 3)
#define MKV_HOFF_FrameRate	(0x180 + 4)
#define MKV_HOFF_MuxingApp	(0x0ea + 3)
#define MKV_HOFF_WritingApp	(0x0fa + 3)
#define MKV_HOFF_CodecID	(0x153 + 2)
#define MKV_HOFF_Language	(0x147 + 4)
#define MKV_HOFF_TrackUID	(0x140 + 3)
#define MKV_HOFF_SegmentUID	(0x10a + 3)
#define MKV_HOFF_FlagLacing	(0x144 + 2)
#define MKV_HOFF_TimecodeScale	(0x0e3 + 4)
/* chunk lengths of interest in mkv_head[] */
#define MKV_CLEN_MuxingApp	(0x0d)
#define MKV_CLEN_WritingApp	(0x0d)

const static uint8_t mkv_head[] = {
	/* EBML */
	0x1a, 0x45, 0xdf, 0xa3,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
		0x42, 0x86, 0x81, 0x01, 0x42, 0xf7, 0x81, 0x01,
		0x42, 0xf2, 0x81, 0x04, 0x42, 0xf3, 0x81, 0x08,
		0x42, 0x82, 0x88, 0x6d, 0x61, 0x74, 0x72, 0x6f,
		0x73, 0x6b, 0x61, 0x42, 0x87, 0x81, 0x02, 0x42,
		0x85, 0x81, 0x02,

	/* L0 Segment (all 1s, thus unknown size) */
	0x18, 0x53, 0x80, 0x67,
	0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	/* L1+ Void */
	0xec,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x93,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00,

	// L1 Info
	0x15, 0x49, 0xa9, 0x66,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45,
		// L2 TimecodeScale
		0x2a, 0xd7, 0xb1,
		0x83,
			0x0f, 0x42, 0x40,
		// L2 MuxingApp
		0x4d, 0x80,
		0x8d,
			// string: "TaniDVR " (...)
			0x54, 0x61, 0x6e, 0x69, 0x44, 0x56, 0x52, 0x20,
			0x30, 0x2e, 0x30, 0x2e, 0x30,
		// L2 WritingApp
		0x57, 0x41,
		0x8d,
			// string: "TaniDVR " (...)
			0x54, 0x61, 0x6e, 0x69, 0x44, 0x56, 0x52, 0x20,
			0x30, 0x2e, 0x30, 0x2e, 0x30,
		// L2 SegmentUID
		0x73, 0xa4,
		0x90,
			0x22, 0xd8, 0x7b, 0xf1, 0xe4, 0xc7, 0xff, 0x82,
			0xbe, 0x50, 0x86, 0x0f, 0x12, 0x9e, 0x76, 0xc2,
		// L2 Duration
		0x44, 0x89,
		0x88,
			0x41, 0x10, 0x3a, 0x60, 0x00, 0x00, 0x00, 0x00,

	/* L1 Tracks */
	0x16, 0x54, 0xae, 0x6b,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70,
		/* L2 TrackEntry */
		0xae,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67,
			/* L3 TrackNumber */
			0xd7, 0x81, 0x01,
			/* L3 TrackUID */
			0x73, 0xc5, 0x81, 0x01,
			/* L3 FlagLacing */
			0x9c, 0x81, 0x00,
			/* L3 Language */
			0x22, 0xb5, 0x9c,
			0x83, 0x75, 0x6e, 0x64,
			/* L3 FlagDefault */
			0x88, 0x81, 0x00,
			/* L3 CodecID */
			0x86,
			0x8f,
				0x56, 0x5f, 0x4d, 0x50, 0x45, 0x47, 0x34, 0x2f,
				0x49, 0x53, 0x4f, 0x2f, 0x41, 0x56, 0x43,
			/* L3 TrackType */
			0x83, 0x81, 0x01,
			/* L3 Video */
			0xe0,
			0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a,
				/* L4 PixelWidth */
				0xb0, 0x82, 0x01, 0x60,
				/* L4 PixelHeight */
				0xba, 0x82, 0x00, 0xf0,
				/* L4 DisplayWidth (display aspect) */
				0x54, 0xb0, 0x82, 0x00, 0x04,
				/* L4 DisplayHeight (display aspect) */
				0x54, 0xba, 0x82, 0x00, 0x03,
				/* L4 FrameRate */
				0x23, 0x83, 0xe3,
				0x84,
					0x3f, 0x80, 0x00, 0x10,
			/* L3 CodecPrivate */
			0x63, 0xa2,
			0x99,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x67, 0x42,
				0xe0, 0x14, 0xda, 0x05, 0x82, 0x17, 0x89, 0x40,
				0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x30, 0xa4,
				0x80
};

static int dt_dhav_to_h264raw (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len);
static int dt_h264raw_to_mkv (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len);

/* specific to cases where dst < src.
   it is expected to be faster than memmove() */
static void memcpy_overlap (uint8_t *dst, uint8_t *src, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		*(dst++) = *(src++);
	}
}

/* search for binary 'seq' sequence of bytes within 'mem' memory.
   requires: mem_len > 0, seq_len > 0, mem !=NULL, seq != NULL
   returns: pointer to first occurence of 'seq' in 'mem', or NULL if not found */
uint8_t *find_bin_seq_in_mem (uint8_t *mem, uint8_t *seq, const size_t mem_len, const size_t seq_len)
{
	size_t i;

	if (seq_len > mem_len)
		return NULL;

	for (i = 0; i < (mem_len - (seq_len - 1)); i++) {
		if (memcmp (mem + i, seq, seq_len) == 0)
			return (mem + i);
	}

	return NULL;
}

/* identify media container format in src, src_len-sized.
   will identify between MC_FORM_RAW_H264/MC_FORM_DHAV.
   returns: mc format, or MC_FORM_DVR_UNKNOWN (unknown, or not enough data to identify) */
t_mc_format identify_mc_format (uint8_t *src, size_t src_len)
{
	/* test: DHAV container */
	if ((find_bin_seq_in_mem (src, \
		"DHAV", src_len, 4) != NULL) && \
		(find_bin_seq_in_mem (src, \
		"dhav", src_len, 4) != NULL)) {

		log_printf (LOGT_INFO, "Media container: DHAV\n");
		return MC_FORM_DHAV;
	}

	/* test: raw h.264 (Dahua-generated RTP-like minimal MPEG) */
	if (find_bin_seq_in_mem (src, \
		"Dahua_ZH", src_len, 8) != NULL) {

		log_printf (LOGT_INFO, "Media container: raw h.264\n");
		log_printf (LOGT_WARNING, \
			"SUPPORT FOR THIS DEVICE'S MEDIA CONTAINER "
			"IS EXPERIMENTAL. TRY CHANGING YOUR DEVICE "
			"(DVR/IP camera) "
			"SETTINGS FROM/TO \"H.264\" or \"MPEG\"\n");
		return MC_FORM_RAW_H264;
	}

	return MC_FORM_DVR_UNKNOWN;
}

t_mc_parms *mc_init (t_mc_format mc_format, bool ntsc_exact_60hz)
{
	t_mc_parms *mc_parms;
	size_t i;

	if ((mc_parms = malloc (sizeof (t_mc_parms))) == NULL)
		return NULL;

	mc_parms->mc_format = mc_format;
	mc_parms->frame_type = FT_UNDEFINED;
	mc_parms->has_v_parms = false;
	mc_parms->v_width = 0;
	mc_parms->v_height = 0;
	mc_parms->v_timestamp = 0;
	mc_parms->dhav_epoch = 0;
	mc_parms->ntsc_exact_60hz = ntsc_exact_60hz;

	/* used for direct-from-DHAV (buggy) v_timestamp calculation */
	mc_parms->v_dhav_ts_prev = 0;
	mc_parms->v_first_frame = true;

	return mc_parms;
}

void mc_close (t_mc_parms *mc_parms)
{
	free (mc_parms);
}

dstf_t *dstf_init (void)
{
	dstf_t *dstf;

	if ((dstf = malloc (sizeof (dstf_t))) == NULL)
		return NULL;

	dstf->sq_p = dstf->sq;
	dstf->sq_offs = 0;
	dstf->sq_len = 0;
	dstf->sq_maxlen = T_MC_PARMS_DHAV_STF;
	return dstf;
}

void dstf_close (dstf_t *dstf)
{
	free (dstf);
}

/* convert dhav stream to individual complete frames
   src may be partial data and/or contain more than one frame.
   src data goes to an internal queue after each call.
   dst will be a single whole frame per call (if src provided enough data).

   if the internal queue contains multiple frames,
   this function must be called multiple times with src_len==0
   until it returns 0 (or <0 if error).
   even when it returns >0 dst_len may be 0 (usually a frame filtered out).

   *** dst may be the same buffer as src.

   returns:
	0, no output frame to dst
	1, written output (single, complete) frame to dst
	2, skipped, filtered out frame (nothing written, but may be other frames still)
	<0, error */
int dstf_process_dhav_stream_to_frames (dstf_t *dstf, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len)
{
	size_t q_free;		/* post-offset free data */
	size_t q_tot_free;	/* all free data after queue packing */
	uint8_t *framep;
	size_t dhav_rep_len;    /* frame length reported by DHAV header */
	size_t dhav_rep2_len;	/* frame length reported by dhav header */
	uint8_t frame_type;
	size_t dhav_offset;	/* offset to beginning of (traling "header") 'd','h','a','v' */
	bool skip_garbage;

	q_free = dstf->sq_maxlen - (dstf->sq_len + dstf->sq_offs);
	q_tot_free = dstf->sq_maxlen - dstf->sq_len;

	*dst_len = 0;

	if (src_len > 0) {
		/* incoming stream data */
		if (src_len > q_tot_free) {
			/* internal error: no buffer space to fit this data. */

			return -1;
		}
		if (src_len > q_free) {
			/* pack queue */
			memcpy_overlap (dstf->sq_p, (dstf->sq_p + dstf->sq_offs), dstf->sq_len);
			dstf->sq_offs = 0;
		}

		/* load new data into queue */
		memcpy ((dstf->sq_p + dstf->sq_offs + dstf->sq_len), src_p, src_len);
		dstf->sq_len += src_len;
	}

	/* process queue data */

	framep = dstf->sq_p + dstf->sq_offs;

	/* inside this loop, checks whether the frame is whole and valid */
	while (1) {

		if (dstf->sq_len < 16) {
			/* insufficient frame data even to get reported DHAV frame size,
			   more data is necessary */
			return 0;
		}

		skip_garbage = false;

		if (BT_IDeqLM_32('D','H','A','V',framep)) {
			/* has DHAV header, perform further frame checking */
			dhav_rep_len = BT_LM2NV_U32(framep + 12);
			dhav_offset = dhav_rep_len - 8;

			/* check if buffer has the whole advertised frame size */
			if (dhav_rep_len > dstf->sq_maxlen) {
				/* error: buffer size is either too small (cannot hold at least a single whole frame)
				   or data is corrupted. -- assume the latter and skip this data */
				log_printf (LOGT_WARNING, "DHAV frame is either too large, or corrupted data - assuming the latter. Skipping garbage...\n");
				skip_garbage = true;
			} else if (dhav_rep_len > dstf->sq_len) {
				/* incomplete frame, more data is necessary */
				return 0;
			} else if (! BT_IDeqLM_32('d','h','a','v', (framep + dhav_offset))) {
				log_printf (LOGT_WARNING, "No dhav trailer. Skipping garbage...\n");
				skip_garbage = true;
			} else if ((dhav_rep2_len = BT_LM2NV_U32(framep + dhav_offset + 4)) != dhav_rep_len) {
				log_printf (LOGT_WARNING, "Corrupt dhav size. Skipping garbage...\n");
				skip_garbage = true;
			} else {
				/* frame is whole and seems correct, continue */
				break;
			}
		} else {
			/* no DHAV header */
			log_printf (LOGT_WARNING, "No DHAV header. Skipping garbage...\n");
			skip_garbage = true;
		}

		/* SKIP GARBAGE */
		if (skip_garbage == true) {
			/* jump to the next position */
			framep++;
			(dstf->sq_offs)++;
			(dstf->sq_len)--;

			/* to save processing, this is done all here:
			   skip up to next DHAV, or until there is no workable data */
			while ((dstf->sq_len >= 16) && (! BT_IDeqLM_32('D','H','A','V',framep))) {
				framep++;
				(dstf->sq_offs)++;
				(dstf->sq_len)--;
			}
			if (dstf->sq_len < 16) {
				/* insufficient data remains for evaluation, more data needed */
				return 3;
			}
		}
	}

	if (max_dst_len < dhav_rep_len) {
		/* insufficient dst buffer for this single frame */
		return -3;
	}

	/* set queue to next frame
	   (does not change queue array itself,
	   and those vars will not be used from now on) */
	dstf->sq_len = dstf->sq_len - dhav_rep_len;
	dstf->sq_offs = dstf->sq_offs + dhav_rep_len;

	/* transfer frame data from queue to dst */
	if (max_dst_len < dhav_rep_len) {
		/* buffer overflow: max_dst_len < dhav_rep_len */
		return -4;
	}
	memcpy (dst_p, framep, dhav_rep_len);
	*dst_len = dhav_rep_len;
	return 1;
}

/* returns the offset (relative to src) of the next NAL sequence (00 00 01),
   -1, if not found */
int search_mpeg_NAL (uint8_t *src, size_t len)
{
	int i = 0;

	while (i < (len - 2)) {
		if ((*(src + i + 0) == 0x00) && \
			(*(src + i + 1) == 0x00) && \
			(*(src + i + 2) == 0x01)) {
			return i;
		}
		i++;
	}
	return -1;
}

/* same as dstf_process_dhav_stream_to_frames,
   except it treats incoming RAW H.264 data */
int dstf_process_raw_h264_stream_to_frames (dstf_t *dstf, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len)
{
	size_t q_free;		/* post-offset free data */
	size_t q_tot_free;	/* all free data after queue packing */
	uint8_t *framep;
	bool skip_garbage;
	int next_NAL;
	size_t frame_len;

	q_free = dstf->sq_maxlen - (dstf->sq_len + dstf->sq_offs);
	q_tot_free = dstf->sq_maxlen - dstf->sq_len;

	*dst_len = 0;

	if (src_len > 0) {
		/* incoming stream data */
		if (src_len > q_tot_free) {
			/* internal error: no buffer space to fit this data. */
			return -1;
		}
		if (src_len > q_free) {
			/* pack queue */
			memcpy_overlap (dstf->sq_p, (dstf->sq_p + dstf->sq_offs), dstf->sq_len);
			dstf->sq_offs = 0;
		}

		/* load new data into queue */
		memcpy ((dstf->sq_p + dstf->sq_offs + dstf->sq_len), src_p, src_len);
		dstf->sq_len += src_len;
	}

	/* process queue data */

	framep = dstf->sq_p + dstf->sq_offs;



	/* checks whether the frame is whole and valid */



	if (dstf->sq_len < 8) {
		/* insufficient data for a whole (0 bytes-sized) frame */
		return 0;
	}

	/* search for first NAL sequence */
	next_NAL = search_mpeg_NAL (framep, dstf->sq_len);
	if (next_NAL < 0) {
		framep += dstf->sq_len;
		(dstf->sq_offs) += dstf->sq_len;
		(dstf->sq_len) = 0;

		/* first NAL not found.
		   insufficient data remains for evaluation, more data needed */
		return 0;
	}
	if (next_NAL != 0) {
		log_printf (LOGT_WARNING, "No NAL sequence. Skipping garbage...\n");
		framep += next_NAL;
		(dstf->sq_offs) += next_NAL;
		(dstf->sq_len) -= next_NAL;
	}

	if (dstf->sq_len < 8) {
		/* insufficient data for finding the frame end */
		return 0;
	}

	/* search for the second NAL sequence */
	next_NAL = search_mpeg_NAL (framep + 4, dstf->sq_len - 4);
	if (next_NAL < 0) {
		/* beginning of next frame not found, unable to
		   determine size of full frame yet, thus it is considered incomplete */
		return 0;
	}
	frame_len = next_NAL + 4;	/* frame_len includes heading NAL */



	/* GOT WHOLE FRAME, continue */



	if (max_dst_len < frame_len) {
		/* insufficient dst buffer for this single frame */
		log_printf (LOGT_ERROR, "Insufficient dst buffer for this single frame.\n");
		return -3;
	}

	/* set queue to next frame
	   (does not change queue array itself,
	   and those vars will not be used from now on) */
	dstf->sq_len = dstf->sq_len - frame_len;
	dstf->sq_offs = dstf->sq_offs + frame_len;

	memcpy (dst_p, framep, frame_len);
	*dst_len = frame_len;

	return 1;
}



/* ********************************* */

/* related to DHAV-subfields (entries within extended header data) */
#define DHAV_SF_STRIPLEN_MAX 8
#define DHAV_SF_ARRAY uint8_t dhav_subfields_array[DHAV_SF_STRIPLEN_MAX * 256]
#define DHAV_SF_DATA(fn,dn) dhav_subfields_array[((fn) << 3) + (dn)]
#define DHAV_SF_LOAD_EXTHEADER_DATA(ehd,ehl) dhav_sf_parse_ext_head ((ehd), (ehl), dhav_subfields_array)

static void dhav_sf_parse_ext_head (uint8_t *ext_head, const uint8_t ext_head_len, uint8_t *dhav_subfields_array)
{
	uint8_t id;
	uint8_t id_len = 0;
	uint8_t id_pos = 0;
	int i;
	int hl = ext_head_len;
	int pos = 0;

	/* zero subfields */
	for (i = 0; i < (DHAV_SF_STRIPLEN_MAX * 256); i++) {
		dhav_subfields_array[i] = 0;		
	}

	while (hl != 0) {
		if (id_len == 0) {
			id_pos = 0;

			id = ext_head[pos];
			id_len = (id == 0x88) ? 8 : 4;	// only subfield 0x88 has 8 bytes, other types: 4 bytes
		}

		DHAV_SF_DATA(id,id_pos) = *(ext_head + pos);

		id_pos++;
		pos++;
		id_len--;
		hl--;
	}
}


#define BASE_DHAV_HDR_LEN 24
/* collect DHAV frame information (timestamp, h264 body size etc)
   this function assumes a complete and correct single DHAV frame from src */
/* if fix_fs == true, correct buggy dhav timestamp */
/* returns ==0 ok, >0 error/warning, <0 fatal error */
int dt_collect_dhav_frame_info (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len)
{
	const size_t dhav_len = 8;	/* trailing chunk 'd','h','a','v' len */
	size_t body_offset;
	size_t body_len;
	uint8_t DHAV_type;
	size_t DHAV_len;
	size_t DHAV_exthead_len;
	uint16_t v_dhav_period;	/* in msec */
	//
	DHAV_SF_ARRAY;	/* see dhav_sf_parse_ext_head and related macros */
#ifdef DEBUG
	int i;
#endif

	mc_parms->frame_type = FT_UNDEFINED;

	if (src_len == 0)
		return 0; /* nothing to do, do nothing */

	/* process frame */

	DHAV_type = *(src_p + 4);
	DHAV_len = *(src_p + 22) + BASE_DHAV_HDR_LEN;
	DHAV_exthead_len = *(src_p + 22);
	body_offset = DHAV_len;
	body_len = src_len - body_offset - dhav_len;

	/* parse extended header data and load into array */
	DHAV_SF_LOAD_EXTHEADER_DATA(src_p+BASE_DHAV_HDR_LEN,DHAV_exthead_len);

#ifdef DEBUG
if (DHAV_type == 0xfd) {
	DEBUG_LOG_PRINTF ("DHAV HEAD DUMP FOLLOWS\n\t----------------------------");
	for (i = 0; i < DHAV_len; i++) {
		if ((i & 0x07) == 0) {
			DEBUG_LOG_PRINTF_RAW ("\n\t%04x ", i);
		}
		DEBUG_LOG_PRINTF_RAW ("%02x ", *(src_p + i));
	}
	DEBUG_LOG_PRINTF_RAW ("\n\t----------------------------\n\n");
}
#endif

	switch (DHAV_type) {
	case 0xfc:
		/* video frame */

		if (mc_parms->has_v_parms == false) {
			/* not an error, this is meant to skip
			   frames until the first I-frame arrives,
			   since certain information is available
			   only in such frames. */
			return 0;
		}
		mc_parms->frame_type = FT_VIDEO_FRAME;
		break;
	case 0xfd:
		/* video I-frame */

		mc_parms->v_width = DHAV_SF_DATA(0x80,0x02) * 8;
		mc_parms->v_height = DHAV_SF_DATA(0x80,0x03) * 8;
		mc_parms->dhav_fps = DHAV_SF_DATA(0x81,0x03);
		if (mc_parms->dhav_fps == 0) {
			log_printf (LOGT_ERROR, "DHAV header with 0 fps defined. This is a serious error, please report this situation to developers.\n");
			return -1;
		}

		/* guess aspect-ratio
		   (there is no aspect-ratio info in DHAV headers) */
		switch ((mc_parms->v_width * 1000) / mc_parms->v_height) {
			case 1777:
				/* 1.777:1.0 AKA 16:9 */
				mc_parms->v_aspect_x = 16;
				mc_parms->v_aspect_y = 9;
				break;
			default:
				/* all other resolutions, including SDTV ones with
				   non-square PAL/NTSC pixel are assumed to be 4:3 */
				mc_parms->v_aspect_x = 4;
				mc_parms->v_aspect_y = 3;
				break;
		}

		/* guesses whether the video came from NTSC source.
		   this information is needed to perform fine framerate adjustment
		   due to NTSC's 59.94/60.0Hz quirk.
		   (DHAV provides no information on whether the source is PAL or NTSC) */
		/* heights -- NTSC: 60,120,160,240,480 ; PAL: 72,144,192,288,576 ; HD PAL/NTSC: other */
		mc_parms->NTSC_timings = false;
		switch (mc_parms->v_height) {
			case 60:
			case 120:
			case 160:
			case 240:
			case 480:
				/* NTSC */
				mc_parms->NTSC_timings = true;
				break;
			case 72:
			case 144:
			case 192:
			case 288:
			case 576:
				/* PAL */
				mc_parms->NTSC_timings = false;
				break;
			default:
				/* resolution does not tell, educated guess then */
				if ((mc_parms->dhav_fps == 60) || \
					(mc_parms->dhav_fps == 30) || \
					(mc_parms->dhav_fps == 15)) {
					mc_parms->NTSC_timings = true;
				}
				break;
		}

		mc_parms->has_v_parms = true;

		mc_parms->frame_type = FT_VIDEO_I_FRAME;
		break;
	case 0xf0:
		/* audio */
		/* TODO: implement support for audio */
		mc_parms->frame_type = FT_AUDIO_FRAME;
		return 0;
		break;
	case 0xf1:
		/* (unknown) */
		/* Present in stream of some IP cameras.
		   Sends periodically non-NULL terminated text data
		   with some realtime information, such as:
		   {"FocusStatus"|:{"AutofocusPeak|":1278}} */
		return 0;
		break;
	default:
		/* unknown type */
		log_printf (LOGT_WARNING, "Unknown DHAV frame type received: %x\n", DHAV_type);
		return 0;
		break;
	}

	/* video parms and timings apply only to video frames,
           audio timings are not supported currently */
	if ((DHAV_type == 0xfd) || (DHAV_type == 0xfc)) {
		/* get other data from DHAV */
		mc_parms->v_dhav_ts = BT_LM2NV_U16(src_p + 20);
		mc_parms->dhav_epoch = BT_LM2NV_U32(src_p + 16);

		/* direct-from-DHAV (buggy) v_timestamp calculation */
		if (mc_parms->v_first_frame == true) {
			mc_parms->v_first_frame = false;
			mc_parms->v_dhav_ts_prev = mc_parms->v_dhav_ts;
			mc_parms->v_timestamp = 0;
		} else {
			v_dhav_period = (mc_parms->v_dhav_ts > mc_parms->v_dhav_ts_prev) ? \
				(mc_parms->v_dhav_ts - mc_parms->v_dhav_ts_prev) : \
				(mc_parms->v_dhav_ts + (0 - mc_parms->v_dhav_ts_prev));

			mc_parms->v_timestamp += ((uint64_t) v_dhav_period * 1000000);
			mc_parms->v_dhav_ts_prev = mc_parms->v_dhav_ts;
		}
	}

	mc_parms->body_p = src_p + body_offset;
	mc_parms->body_len = body_len;

	return 0;
}

/* analogous to dt_collect_dhav_frame_info() */
/* collect RAW H.264 (NAL-prefixed) frame information (timestamp, h264 body size etc)
   this function assumes a complete and correct single NAL frame from src */
/* if fix_fs == true, correct buggy timestamp (NOT IMPLEMENTED YET) */
/* returns ==0 ok, >0 error/warning, <0 fatal error */
int dt_collect_raw_h264_frame_info (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len)
{

	const size_t NAL_len = 4;	/* NAL prefix + type: 00 00 01 xx */
	size_t body_offset;
	size_t body_len;
	uint8_t NAL_type;

	mc_parms->frame_type = FT_UNDEFINED;

	if (src_len == 0)
		return 0; /* nothing to do, do nothing */

	/* process frame */

	NAL_type = *(src_p + 3);
	body_offset = NAL_len;
	body_len = src_len - body_offset;

	//mc_parms->body_p = src_p + body_offset;
	//mc_parms->body_len = body_len;

	mc_parms->body_p = src_p;
	mc_parms->body_len = src_len;

	switch (NAL_type) {
	case 0xb6:	// video frame (h264 I/P/B/S-frame)
		switch ((*(src_p + 4 + 0) >> 6) & 0x03) {
		case 0:	// I-frame
			mc_parms->frame_type = FT_VIDEO_I_FRAME;
			break;
		case 1: // P-frame
		case 2: // B-frame
		case 3: // S-frame
		default:
			mc_parms->frame_type = FT_VIDEO_FRAME;
			break;
		}
		break;
	case 0xf0:	// audio frame (PCM?)
		/* TODO: implement support for audio */
		mc_parms->frame_type = FT_AUDIO_FRAME;
		return 0;
		break;
	case 0xfb:	// config: video geometry etc
		mc_parms->v_width = *(src_p + 4 + 2) * 8;
		mc_parms->v_height = *(src_p + 4 + 3) * 8;
		mc_parms->dhav_fps = 20;	// FIXME: BOGUS FPS (unknown source or absent data)

		/* guess aspect-ratio
		   (there is no aspect-ratio info in DHAV headers) */
		switch ((mc_parms->v_width * 1000) / mc_parms->v_height) {
			case 1777:
				/* 1.777:1.0 AKA 16:9 */
				mc_parms->v_aspect_x = 16;
				mc_parms->v_aspect_y = 9;
				break;
			default:
				/* all other resolutions, including SDTV ones with
				   non-square PAL/NTSC pixel are assumed to be 4:3 */
				mc_parms->v_aspect_x = 4;
				mc_parms->v_aspect_y = 3;
				break;
		}

		/* guesses whether the video came from NTSC source.
		   this information is needed to perform fine framerate adjustment
		   due to NTSC's 59.94/60.0Hz quirk.
		   (DHAV provides no information on whether the source is PAL or NTSC) */
		/* heights -- NTSC: 60,120,160,240,480 ; PAL: 72,144,192,288,576 ; HD PAL/NTSC: other */
		mc_parms->NTSC_timings = false;
		switch (mc_parms->v_height) {
			case 60:
			case 120:
			case 160:
			case 240:
			case 480:
				/* NTSC */
				mc_parms->NTSC_timings = true;
				break;
			case 72:
			case 144:
			case 192:
			case 288:
			case 576:
				/* PAL */
				mc_parms->NTSC_timings = false;
				break;
			default:
				/* resolution does not tell, educated guess then */
				if ((mc_parms->dhav_fps == 60) || \
					(mc_parms->dhav_fps == 30) || \
					(mc_parms->dhav_fps == 15)) {
					mc_parms->NTSC_timings = true;
				}
				break;
		}

		mc_parms->has_v_parms = true;
		break;

	case 0x00:	// (unknown)
	case 0x20:	// (unknown)
	case 0xb0:	// (unknown)
	case 0xb2:	// "Dahua_ZH" string (not used)
	case 0xb5:	// (unknown)
	case 0xfa:	// offset to next 0x0fa (not used)
		return 0; /* nothing to do with such frames */
		break;
	default:	// anything else (not used)
		/* unknown type */
		log_printf (LOGT_WARNING, "Unknown H.264 NAL type received: %hhx\n", NAL_type);
		return 0;
		break;
	}

	
	/* video parms and timings apply only to video frames,
           audio timings are not supported currently */
	if (NAL_type = 0xb6) {
		if (mc_parms->v_first_frame == true) {
			mc_parms->v_timestamp = 0;
			mc_parms->v_first_frame = false;
		} else {
			mc_parms->v_timestamp += 60 * 1000 * 1000;	// FIXME: frame period fixed at 60us
		}
	}

	return 0;
}





/* ******* */

/* damps up to +-[frames] of jitter */
#define TIMESTAMP_JITTER_DAMPING_LIMIT 4

#define SLOW_DRIFT_EVAL_MIN_SAMPLES 1000

/* max DHAV epoch jump (in seconds) before
   adjusting timestamp from epoch.
   this CANNOT be higher than ~60s ; (2e16 - 1) ms */
#define EPOCH_MAX_FORWARD_JUMP 30

/* max seconds of epoch difference so to keep
   an exact v_timestamp, instead of just
   incrementing with frame period. */
#define MAX_EPOCH_PEDANTRY_TOWARDS_TIMESTAMP 5

/* this corrects the buggy DVR timestamp.
   call this each time after dstf_process_dhav_stream_to_frames
   provides a whole frame.
   update t_mc_parms->v_timestamp from tsc->v_timestamp to
   apply the corrected value.

   timestamp correction applies to video only
   (audio would require independent processing,
   what is not done currently)

   correction is applied to t_mc_parms ONLY,
   DHAV headers are NOT modified nor readen.

   if input frame is not video, nothing is done. */
/* returns: 0==ok, >0 warning, <0 error */
int dt_tsproc_process (t_mc_tsproc *tsc)
{
	const t_mc_parms *mcp = tsc->mc_parms;	/* alias */
	uint16_t dhav_cur_ts;	/* in msec */
	uint64_t dhav_epoch_cur;
	uint64_t dhav_epoch_prev;
	uint64_t dhav_epoch_diff;	/* negative diffs are treated differently */
	uint16_t dhav_ts_cur;
	uint16_t dhav_ts_prev;

	uint16_t dhav_period_ms;	/* from which dhav_period is (typically) defined, in msec */
	int64_t dhav_period;		/* in nsec */
	int64_t ref_period;		/* reference frame period, in nsec */
	int64_t dhav_avg_period_ns;	/* average DHAV period, in nsec */
	int64_t ref_avg_period_ns;	/* average reference period, in nsec */
	int64_t eff_avg_period_ns;	/* effective (post-correction) period, in nsec */
	int64_t tot_period_diff_eff;	/* slow drift accumulated total difference (dhav vs effective), in nsec */
	int64_t tot_period_diff_eff_abs;	/* == abs(tot_period_diff_eff) */
	int64_t tot_period_diff_ref;	/* slow drift accumulated total difference (dhav vs reference), in nsec */
	int64_t tot_period_diff_ref_abs;	/* == abs(tot_period_diff_ref) */
	int64_t period_to_use;		/* the period to be applied directly over tsc->v_timestamp */
	int64_t timestamp_drift;
	int64_t timestamp_drift_abs;
	bool p_reset_sf = false;	/* reset slow-drift correction stats */

	uint64_t corrected_period;
	int i;
	int64_t j, j_abs;


	if ( (mcp->frame_type != FT_VIDEO_I_FRAME) && \
	   (mcp->frame_type != FT_VIDEO_FRAME) ) {
		/* only h264 video frames are supported */
		return 2;
	}
	if (mcp->has_v_parms == false) {
		/* dt_collect_dhav_frame_info() was
		   unable to get enough video parameters yet */
		return 1;
	}

	if (tsc->v_first_frame == true) {
		tsc->v_first_frame = false;

		tsc->v_dhav_timestamp = 0; /* calculated, does not reflect DHAV headers directly */
		tsc->v_timestamp = 0;
		tsc->sdc_t_dhav = 0;
		tsc->sdc_t_ref = 0;
		tsc->sdc_t_eff = 0;
		tsc->sdc_t_frames = 0;

		/* these will be used by 2nd frame
		   as previous values */
		tsc->dhav_epoch = mcp->dhav_epoch;
		tsc->v_dhav_ts = mcp->v_dhav_ts;

		return 0;
	}

	/* from here, actions performed on the 2nd frame onwards */

	/* synchronizes tsc data from mcp and
	   work with those local vars from now on. */
	tsc->dhav_epoch_prev = dhav_epoch_prev = tsc->dhav_epoch;
	tsc->dhav_epoch = dhav_epoch_cur = mcp->dhav_epoch;
	tsc->v_dhav_ts_prev = dhav_ts_prev = tsc->v_dhav_ts;
	tsc->v_dhav_ts = dhav_ts_cur = mcp->v_dhav_ts;

	ref_period = (((mcp->NTSC_timings == true) && (mcp->ntsc_exact_60hz == false)) ? 1001000000 : 1000000000) / (uint64_t) (mcp->dhav_fps);
	dhav_period_ms = (uint16_t) dhav_ts_cur - (uint16_t) dhav_ts_prev;
	dhav_period = (uint64_t) dhav_period_ms * 1000000;

	/* defaults to 1-frame period */
	period_to_use = ref_period;

	/* check EPOCH jump */
	/* DHAV's relative timestamp cannot tell anything more than ~60s (2e16 - 1)
	   of absolute timestamp difference; if difference is in order of seconds (or more),
	   this uses DHAV EPOCH instead (which has a 1s resolution). */
	/* TODO: obtain (indirectly) a more precise timestamp calculation from
		 DHAV timestamp (by indirect calculation).
	   the problem of _not_ doing so is that, after several time jumps over several hours,
	   fractions of second accumulate -- that causes a very, very slow timestamp drift. */
	dhav_epoch_diff = dhav_epoch_cur - dhav_epoch_prev;
	if (dhav_epoch_cur < dhav_epoch_prev) {
		log_printf (LOGT_WARNING, "DVR time/date went backwards (%lld s). Assuming that time as current from now on.\n", (long long int) (dhav_epoch_prev - dhav_epoch_cur));

		/* DHAV timestamp and its period are worthless now, resync both */
		tsc->v_dhav_timestamp = tsc->v_timestamp;
		dhav_period = period_to_use;;

		p_reset_sf = true;
		goto last_actions;
	}
	if (dhav_epoch_diff > EPOCH_MAX_FORWARD_JUMP) {
		if (dhav_epoch_diff > MAX_EPOCH_PEDANTRY_TOWARDS_TIMESTAMP) {
			/* do not enforce epoch difference
			   into v_timestamp */
			log_printf (LOGT_WARNING, "DVR time/date advanced too far (%lld s). Assuming that time as current from now on.\n", (long long int) dhav_epoch_diff);

			/* DHAV timestamp and its period are worthless now, resync both */
			tsc->v_dhav_timestamp = tsc->v_timestamp;
			dhav_period = period_to_use;;

			p_reset_sf = true;
			goto last_actions;
		}

		log_printf (LOGT_WARNING, "DVR time/date jumped (%lld s). Corrected.\n", (long long int) dhav_epoch_diff);

		/* j == requited whole frames to compensate this drift.
		   (frame frequency phase is maintained) */
		j = (dhav_epoch_diff * 1000000000) / ref_period;
		period_to_use = (j + 1) * ref_period;	/* 1 frame (pending) timestamp + correction */

		/* DHAV timestamp and its period are worthless now, resync both */
		tsc->v_dhav_timestamp = tsc->v_timestamp;
		dhav_period = period_to_use;

		p_reset_sf = true;
		goto last_actions;
	}

	/* check if timestamps differences exceed the tolerance period.
	   if it does, assumes that timestamp is correct (or "more correct"
	   than reference clock).
	   when it may happen (examples):
		- frames were dropped by DVR itself
		- temporary DVR-to-client data disconnection  */
	timestamp_drift =  (tsc->v_dhav_timestamp + dhav_period) - (tsc->v_timestamp + ref_period);
	timestamp_drift_abs = (timestamp_drift >= 0) ? timestamp_drift : (0 - timestamp_drift);
	if (timestamp_drift_abs > (TIMESTAMP_JITTER_DAMPING_LIMIT * ref_period)) {
		log_printf (LOGT_WARNING, "Timestamp drift detected (%lld ms). Corrected.\n", (long long int) timestamp_drift / 1000000);

		/* j == requited whole frames to compensate this drift.
		   (frame frequency phase is maintained) */
		j = timestamp_drift / ref_period;
		period_to_use = ref_period + (j * ref_period);	/* 1 frame (pending) timestamp + correction (+/-) */
		p_reset_sf = true;
		goto last_actions;
	}

	/* timestamp slow drift detection */
	/* update stats */
	tsc->sdc_t_dhav += dhav_period;
	tsc->sdc_t_ref += ref_period;
	tsc->sdc_t_eff += ref_period;
	(tsc->sdc_t_frames)++;
	/* check if slow drift reached a whole frame period */
	dhav_avg_period_ns = (int64_t) ((long double) tsc->sdc_t_dhav / (long double) tsc->sdc_t_frames);
	ref_avg_period_ns = (int64_t) ((long double) tsc->sdc_t_ref / (long double) tsc->sdc_t_frames);
	eff_avg_period_ns = (int64_t) ((long double) tsc->sdc_t_eff / (long double) tsc->sdc_t_frames);

	tot_period_diff_eff = tsc->sdc_t_dhav - tsc->sdc_t_eff;
	tot_period_diff_eff_abs = (tot_period_diff_eff >= 0) ? tot_period_diff_eff : (0 - tot_period_diff_eff);
	tot_period_diff_ref = tsc->sdc_t_dhav - tsc->sdc_t_ref;
	tot_period_diff_ref_abs = (tot_period_diff_ref >= 0) ? tot_period_diff_ref : (0 - tot_period_diff_ref);

	if ((tot_period_diff_eff_abs > (ref_period * 2)) && (tsc->sdc_t_frames >= SLOW_DRIFT_EVAL_MIN_SAMPLES)) {

		/* j == requited whole frames to compensate this drift.
		   (frame frequency phase is maintained) */
		j = tot_period_diff_eff / ref_period;
		j_abs = (j >= 0) ? j : (0 - j);
		period_to_use = ref_period + (j * ref_period);	/* 1 frame (pending) timestamp + correction (+/-) */

		log_printf (LOGT_WARNING, "Timestamp slow drift detected (%lld ms). %s\n", \
			(long long int) (tot_period_diff_eff / 1000000), \
			((j_abs > 2) ? "Corrected (drift >1 frame)." : "Corrected as 1 frame drift."));
		log_printf (LOGT_INFO, "FPS stats - Measured from DVR: %.6f fps (%lld frames) - TaniDVR assumes: %.6f fps - Post-correction: %.6f fps.\n", \
			(float) ((long double) 1.0 / ((long double) dhav_avg_period_ns / 1000000000)), \
			(long long int) tsc->sdc_t_frames, \
			(float) ((long double) 1.0 / ((long double) ref_avg_period_ns / 1000000000)), \
			(float) ((long double) 1.0 / ((long double) eff_avg_period_ns / 1000000000)));


		if (j_abs > 2) {
			/* too much discrepancy, let's be safe and reset stats */
			period_to_use = ref_period + (j * ref_period);  /* 1 frame (pending) timestamp + correction (+/-) */
			p_reset_sf = true;
		} else {
			/* here, j == 2  always */
			/* assumes 1 frame of real drift and 1 frame caused by jitter; corrects just one. */

			period_to_use = ref_period + (1 * ref_period);

			/* adjust stats, considering drift correction */
			tsc->sdc_t_eff = tsc->sdc_t_eff + period_to_use - ref_period;
		}
		goto last_actions;
	}

last_actions:

	if (p_reset_sf == true) {
		/* reset slow-drift stats */
		tsc->sdc_t_dhav = 0;
		tsc->sdc_t_ref = 0;
		tsc->sdc_t_eff = 0;
		tsc->sdc_t_frames = 0;
	}

	/* increment v_timestamp with the time lapse
	   chosen as correct */
	tsc->v_timestamp += period_to_use;
	tsc->v_dhav_timestamp += dhav_period;
}

/* initialize DHAV timestamp processing/correction structure.
   requires initialized t_mc_parms. */
t_mc_tsproc *dt_tsproc_init (const t_mc_parms *mcp, tsproc_t tsproc)
{
	t_mc_tsproc *tsc;

	if ((tsc = malloc (sizeof (t_mc_tsproc))) == NULL)
		return NULL;

	tsc->tsproc = tsproc;	/* only one type, currently ignored */
	tsc->mc_parms = mcp;
	tsc->v_first_frame = true;

	return tsc;
}

void dt_tsproc_close (t_mc_tsproc *tsc)
{
	free (tsc);
}

/* convert int FPS to float stored in u32b.
   ASSUMES C99/IEEE-754 environment. */
static uint32_t transform_fps_to_mkvfps (const uint8_t fps_in, const bool NTSC_timings, const bool ntsc_exact_60hz)
{
	union {
	        uint32_t d;
	        float f;
	} fps;

	fps.f = (float) ((double) fps_in * \
		(((NTSC_timings == true) && (ntsc_exact_60hz == true)) ? \
		(double) 1 : ((double) 1000 / (double) 1001)));
	return fps.d;	
}

/* converts DHAV to MKV+H.264
   REQUIRES: src_p != dst_p
   this function assumes a complete and correct single DHAV frame from src */
/* timestamp in micro-seconds (not mili-seconds) */
/* sequence - sequential number for each frame */
/* ATTENTION: this function must be called with first_frame==true
   until it returns ==0, then use first_frame==false */
/* returns ==0 ok ; <0 fatal error ; >0 soft error, warning */
#define WHOLE_CLUSTER_HEADER_LOAD 22
#define WHOLE_SIMPLEBLOCK_HEADER_LOAD 13
int dt_convert_frame_to_mkv (const t_mc_parms *mc_parms, uint8_t *src_dhav_p, size_t src_dhav_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len, bool first_frame, mcodec_t vcodec)
{
	uint8_t *src_p;
	size_t src_len;
	uint64_t timestamp_us = mc_parms->v_timestamp / 1000;
	size_t mkv_cluster_len;
	size_t mkv_simpleblock_len;
	size_t whole_payload;
	uint32_t fps_dhav_f;
	int x, y;
	const char tanidvr_sw_str[] = "TaniDVR "VERSION"\0";
	const char str_MCODEC_V_MPEG4_ISO_AVC[32] = "V_MPEG4/ISO/AVC\0";
	const char str_MCODEC_V_MPEG4_ISO_ASP[32] = "V_MPEG4/ISO/ASP\0";

	*dst_len = 0;

	if (src_dhav_len == 0)
		return 3; /* nothing to do, do nothing */

	if ( (mc_parms->frame_type != FT_VIDEO_I_FRAME) && \
	   (mc_parms->frame_type != FT_VIDEO_FRAME) ) {
		/* only h264 video frames are supported */
		return 2;
	}
	if (mc_parms->has_v_parms == false) {
		/* dt_collect_dhav_frame_info() was
		   unable to get enough video parameters yet */
		return 1;
	}

	/* only the body of DHAV frame (the h264 data itself)
           will be used from src_dhav */
	src_p = mc_parms->body_p;
	src_len = mc_parms->body_len;

	whole_payload = WHOLE_CLUSTER_HEADER_LOAD + WHOLE_SIMPLEBLOCK_HEADER_LOAD + src_len;

	/* if first frame, add main MKV header */
	if ((first_frame == true) && (mc_parms->has_v_parms == true)) {
		whole_payload += WHOLE_MAIN_HEADER_LOAD;
		memcpy (dst_p, mkv_head, WHOLE_MAIN_HEADER_LOAD);

		/* update video codec parameter into main MKV header */
		if (vcodec == MCODEC_V_MPEG4_ISO_AVC) {
			memcpy (dst_p + MKV_HOFF_CodecStr, str_MCODEC_V_MPEG4_ISO_AVC, 15);
		} else {
			memcpy (dst_p + MKV_HOFF_CodecStr, str_MCODEC_V_MPEG4_ISO_ASP, 15);
		}

		/* update video parameters into main MKV header */
		BT_NV2MM_U16(dst_p + MKV_HOFF_PixelWidth, (uint16_t) mc_parms->v_width);
		BT_NV2MM_U16(dst_p + MKV_HOFF_PixelHeight, (uint16_t) mc_parms->v_height);
		BT_NV2MM_U16(dst_p + MKV_HOFF_DisplayWidth, (uint16_t) mc_parms->v_aspect_x);
		BT_NV2MM_U16(dst_p + MKV_HOFF_DisplayHeight, (uint16_t) mc_parms->v_aspect_y);

		/* convert DHAV FPS to IEEE-754 float to be written into MKV main header */
		fps_dhav_f = transform_fps_to_mkvfps (mc_parms->dhav_fps, mc_parms->NTSC_timings, mc_parms->ntsc_exact_60hz);
		BT_NV2MM_U32(dst_p + MKV_HOFF_FrameRate, fps_dhav_f);

		/* update software strings in MKV header */
		x = strlen (tanidvr_sw_str);
		/* MuxingApp */
		memset (dst_p + MKV_HOFF_MuxingApp, 0, MKV_CLEN_MuxingApp);
		strncpy (dst_p + MKV_HOFF_MuxingApp, tanidvr_sw_str, ((x > MKV_CLEN_MuxingApp) ? MKV_CLEN_MuxingApp : x));
		/* WritingApp */
		memset (dst_p + MKV_HOFF_WritingApp, 0, MKV_CLEN_WritingApp);
		strncpy (dst_p + MKV_HOFF_WritingApp, tanidvr_sw_str, ((x > MKV_CLEN_WritingApp) ? MKV_CLEN_WritingApp : x));

		/* Language */
		*(dst_p + MKV_HOFF_Language) = (uint8_t) 'u';
		*(dst_p + MKV_HOFF_Language + 1) = (uint8_t) 'n';
		*(dst_p + MKV_HOFF_Language + 2) = (uint8_t) 'd';

		/* TimecodeScale (1 us) */
		*(dst_p + MKV_HOFF_TimecodeScale) = (uint8_t) 0x00;
		*(dst_p + MKV_HOFF_TimecodeScale + 1) = (uint8_t) 0x03;
		*(dst_p + MKV_HOFF_TimecodeScale + 2) = (uint8_t) 0xe8;

		dst_p += WHOLE_MAIN_HEADER_LOAD;
	}

	if (whole_payload > max_dst_len)
		return -4; /* output buffer is too short */

	/* MKV Cluster */
	*(dst_p++) = 0x1f; // cluster ID
	*(dst_p++) = 0x43; // cluster ID
	*(dst_p++) = 0xb6; // cluster ID
	*(dst_p++) = 0x75; // cluster ID

	/* cluster size (EMBL) */
	/* cluster size = sizeof (whole cluster) - sizeof (cluster ID) - sizeof (cluster size)
	   -> cluter size = sizeof (whole cluster) - 12; */
	mkv_cluster_len = src_len + WHOLE_CLUSTER_HEADER_LOAD + WHOLE_SIMPLEBLOCK_HEADER_LOAD - 12;
	*(dst_p++) = 0x01;
	*(dst_p++) = (mkv_cluster_len >> 48) & 0xff;
	*(dst_p++) = (mkv_cluster_len >> 40) & 0xff;
	*(dst_p++) = (mkv_cluster_len >> 32) & 0xff;
	*(dst_p++) = (mkv_cluster_len >> 24) & 0xff;
	*(dst_p++) = (mkv_cluster_len >> 16) & 0xff;
	*(dst_p++) = (mkv_cluster_len >> 8) & 0xff;
	*(dst_p++) = mkv_cluster_len & 0xff;

	*(dst_p++) = 0xe7; // timecode ID

	*(dst_p++) = 0x88; // sizeof abs timestamp (EMBL)

	/* absolute timestamp (micro-seconds) */
	*(dst_p++) = (timestamp_us >> 56) & 0xff;
	*(dst_p++) = (timestamp_us >> 48) & 0xff;
	*(dst_p++) = (timestamp_us >> 40) & 0xff;
	*(dst_p++) = (timestamp_us >> 32) & 0xff;
	*(dst_p++) = (timestamp_us >> 24) & 0xff;
	*(dst_p++) = (timestamp_us >> 16) & 0xff;
	*(dst_p++) = (timestamp_us >> 8) & 0xff;
	*(dst_p++) = timestamp_us & 0xff;

	/* MKV SimpleBlock */
	*(dst_p++) = 0xa3; // simpleblock ID

	/* track length (EMBL) */
	mkv_simpleblock_len = src_len + WHOLE_SIMPLEBLOCK_HEADER_LOAD - 9;
	*(dst_p++) = 0x01;
	*(dst_p++) = (mkv_simpleblock_len >> 48) & 0xff;
	*(dst_p++) = (mkv_simpleblock_len >> 40) & 0xff;
	*(dst_p++) = (mkv_simpleblock_len >> 32) & 0xff;
	*(dst_p++) = (mkv_simpleblock_len >> 24) & 0xff;
	*(dst_p++) = (mkv_simpleblock_len >> 16) & 0xff;
	*(dst_p++) = (mkv_simpleblock_len >> 8) & 0xff;
	*(dst_p++) = mkv_simpleblock_len & 0xff;

	*(dst_p++) = 0x81; // ??? (EMBL?)
	/* note: since each frame has its cluster
           (thus its own timestamp, we will not use
           the relative one (sets to 0x0000) */
	*(dst_p++) = 0x00; /* relative timestamp MSB */ // era 0x00
	*(dst_p++) = 0x00; /* relative timestamp LSB */
	*(dst_p++) = (mc_parms->frame_type == FT_VIDEO_I_FRAME) ? 0x01 : 0x00; /* flags */

	/* copy H.264 data */
	memcpy (dst_p, src_p, src_len);
	dst_p += src_len;;

	*dst_len = whole_payload;

	return 0;
}

