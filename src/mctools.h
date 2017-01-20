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

// FIXME: this name should be updated
#ifndef HAS_DHAVTOOLS_H
#define HAS_DHAVTOOLS_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>

#define T_MC_PARMS_DHAV_STF 1000000

#define TIMESTAMP_DRIFT_EVAL_MIN_SAMPLES 1000

typedef enum {
	MC_FORM_DHAV,
	MC_FORM_MKV,
	MC_FORM_RAW_H264,
	MC_FORM_DVR_NATIVE,
	MC_FORM_DVR_UNKNOWN
} t_mc_format;

typedef enum {
	FT_UNDEFINED,
	FT_VIDEO_FRAME,
	FT_VIDEO_I_FRAME,
	FT_AUDIO_FRAME
} t_frame_type;

typedef enum {
	TSPROC_NONE,
	TSPROC_DO_CORRECT
} tsproc_t;

typedef enum {
	MCODEC_V_MPEG4_ISO_AVC,
	MCODEC_V_MPEG4_ISO_ASP
} mcodec_t;

typedef struct {
	t_mc_format mc_format;	/* target container format */
	bool v_first_frame;	/* used internally - true: pending or currently processing first usable frame (happens to be an I-frame) */
	bool ntsc_exact_60hz;	/* (CLI option) assumes NTSC with exact 60.0 Hz, instead of 59.94 Hz */

	t_frame_type frame_type;	/* current frame type */

	bool has_v_parms;	/* if true, this struct has valid v_* data (only after first I-frame) */
	uint32_t v_width;	/* in pixels */
	uint32_t v_height;	/* in pixels */
	uint64_t v_aspect_x;	/* usually 4 (for 4:3) */
	uint64_t v_aspect_y;	/* usually 3 (for 4:3) */
	uint32_t dhav_epoch;	/* DHAV EPOCH for this frame */
	uint64_t v_timestamp;	/* absolute timestamp calculated from DHAV, in nsec - starts from 0 */
	uint16_t v_dhav_ts;	/* DHAV (truncated, relative) timestamp collected from frame header (mili-seconds) */
	bool NTSC_timings;	/* true: video source is NTSC ; false: video source is PAL */
	uint8_t dhav_fps;	/* frames/sec as reported by DHAV */

	/* used for direct-from-DHAV (buggy) v_timestamp calculation */
	uint16_t v_dhav_ts_prev;	/* dhav previous frame timestamp, does not start with 0, in msec */

	uint8_t *body_p;	/* pointer to raw h264 data, audio or other raw media inside the DHAV frame */
	size_t body_len;	/* length of body data */
} t_mc_parms;

typedef struct {
	tsproc_t tsproc;	/* type of timestamp correction to be performed */

	const t_mc_parms *mc_parms;	/* get frame information from this. frame MUST be complete. */
	bool v_first_frame;	/* used internally - true: pending or currently processing first usable frame (happens to be an I-frame) */

	uint64_t v_dhav_timestamp;	/* accumulated (exact, dhav source) timestamp, in nsec - starts from 0 */
	uint64_t v_timestamp;	/* accumulated (jitter-less, effective output) timestamp, in nsec - starts from 0 */

	/* slow drift compensation */
	int64_t sdc_t_dhav;	/* accumulated DHAV frame period - nsec */
	int64_t sdc_t_ref;	/* accumulated reference frame period - nsec */
	int64_t sdc_t_eff;	/* accumulated effective (post-correction) frame period */
	int64_t sdc_t_frames;	/* total frames so far */

	/* those are kept synchronized with data from t_mc_parms */
	uint16_t v_dhav_ts;	/* DHAV (truncated, relative) timestamp collected from frame header (mili-seconds) */
	uint16_t v_dhav_ts_prev;	/* dhav previous frame timestamp, does not start with 0, in msec */
	uint32_t dhav_epoch;		/* DHAV EPOCH for this frame */
	uint32_t dhav_epoch_prev;	/* DHAV EPOCH of previous frame */
} t_mc_tsproc;

/* frame-type mask (see ftmask) */
#define MC_FM_VIDEO_FRAME	(1<<0)
#define MC_FM_VIDEO_I_FRAME	(1<<1)
#define MC_FM_AUDIO_FRAME	(1<<2)
#define MV_FM_ALL		(0xffffffff)

/* used by DHAV/H.264 stream to frames converter */
typedef struct {
	uint8_t sq[T_MC_PARMS_DHAV_STF];
	uint8_t *sq_p;
	size_t sq_offs;	/* offset. -- start of valid data = (sq_p + sq_offs) */
	size_t sq_len;	/* useful data. -- boundary of used data = (sq_len + sq_offs + sq_p) */
	size_t sq_maxlen;
} dstf_t;


extern t_mc_format identify_mc_format (uint8_t *src, size_t src_len);

extern int dt_collect_dhav_frame_info (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len);
extern int dt_collect_raw_h264_frame_info (t_mc_parms *mc_parms, uint8_t *src_p, size_t src_len);

extern t_mc_parms *mc_init (t_mc_format mc_format, bool assume_ntsc60hz);
extern void mc_close (t_mc_parms *mc_parms);
extern int dt_convert_frame_to_mkv (const t_mc_parms *mc_parms, uint8_t *src_dhav_p, size_t src_dhav_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len, bool first_frame, mcodec_t vcodec);

extern dstf_t *dstf_init (void);
extern void dstf_close (dstf_t *dstf);
extern int dstf_process_dhav_stream_to_frames (dstf_t *dstf, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len);
int dstf_process_raw_h264_stream_to_frames (dstf_t *dstf, uint8_t *src_p, size_t src_len, uint8_t *dst_p, size_t *dst_len, size_t max_dst_len);

extern int dt_tsproc_process (t_mc_tsproc *tsc);
extern t_mc_tsproc *dt_tsproc_init (const t_mc_parms *mcp, tsproc_t tsproc);
extern void dt_tsproc_close (t_mc_tsproc *tsc);

#endif

