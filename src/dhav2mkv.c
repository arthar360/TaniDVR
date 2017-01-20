/* dhav2mkv standalone tool */

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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>

#include "log.h"
#include "filetools.h"
#include "mctools.h"
#include "dvrcontrol.h"
#include "config.h"	/* autotools-generated */

/* this buffer MUST be > than T_MC_PARMS_DHAV_STF */
#define STREAM_IN_BUFFER_LEN 2000000
/* this MUST be <= than T_MC_PARMS_DHAV_STF */
#define STREAM_IN_FREAD_GRANULARITY 10000

/* must the large enough to hold at least one whole frame */
/* this buffer MUST > than STREAM_IN_BUFFER_LEN */
#define STREAM_OUT_BUFFER_LEN 3000000


struct struct_command_options {
	int dvr_channel;
	const char *in_file;		/* never NULL, if=="\0" uses stdin instead */
	const char *out_file;		/* never NULL, if=="\0" uses stdout instead */
	bool ntsc_exact_60hz;
	tsproc_t tsproc;
} command_options;


void process_command_line_arguments (int argc, char **argv)
{
	int option_index = 0;
	int option;
	bool defined_operation_mode = false;
	bool defined_dvr_target = false;
	bool defined_dvr_user = false;
	bool defined_dvr_password = false;
	int p;

	struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"dvr-channel", 1, 0, 'c'},
		{"in-file", 1, 0, 'i'},
		{"out-file", 1, 0, 'o'},
		{"sixty-hertz-ntsc", 0, 0, 'x'},
		{"ts-proc", 1, 0, 'r'},
		{0, 0, 0, 0}
	};

	command_options.in_file = "\0";
	command_options.out_file = "\0";
	command_options.ntsc_exact_60hz = false;
	command_options.tsproc = TSPROC_DO_CORRECT;

	while ((option = getopt_long (argc, argv, "hc:i:o:xr:", long_options, &option_index)) != EOF) {
		switch (option) {
			case 'h':
				printf ("dhav2mkv " VERSION "\n"
						"Copyright (c) 2011-2015 Daniel Mealha Cabrita\n"
						"\n"
						"This program is free software: you can redistribute it and/or modify\n"
						"it under the terms of the GNU General Public License as published by\n"
						"the Free Software Foundation, either version 3 of the License, or\n"
						"(at your option) any later version.\n"
						"\n"
						"This program is distributed in the hope that it will be useful,\n"
						"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
						"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
						"GNU General Public License for more details.\n"
						"\n"
						"You should have received a copy of the GNU General Public License\n"
						"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
						"\n\n"
						
						"Usage: dhav2mkv [-i <input DHAV file>] [-o <output MKV file>] [(...)] [-h]\n\n"
						"-i, --in-file\n\t<input DHAV file> (default: empty -- uses stdin)\n\n"
						"-o, --out-file\n\t<output MKV file> (default: empty -- uses stdout)\n\n"
						"-x, --sixty-hertz-ntsc\n\t(default: not enabled)\n"
							"\tIf defined, assumes NTSC field frequency to be 60Hz,\n"
							"\tinstead of the typical 59.94Hz.\n"
							"\tSome cameras do generate exact 60Hz video.\n"
							"\tThis switch does NOT affect PAL video.\n\n"
						"-r, --ts-proc\n"
							"\t0 - No correction will be performed to\n"
							"\t    the buggy DHAV stream timestamps\n"
							"\t1 - Perform timestamp correction (default)\n\n"
						"-h, --help\n\tDisplay help text (this one).\n\n"
						"\n");
				exit (0);
				break;
			case 'c':
				log_printf (LOGT_WARNING, "Option -c, --dvr-channel is obsolete and takes no effect.\n");
				break;
			case 'i':
				command_options.in_file = optarg;
				break;

			case 'o':
				command_options.out_file = optarg;
				break;
			case 'x':
				command_options.ntsc_exact_60hz = true;
				break;
			case 'r':
				sscanf (optarg, "%d", &p);
				switch (p) {
				case 0:	command_options.tsproc = TSPROC_NONE;		break;
				case 1:	command_options.tsproc = TSPROC_DO_CORRECT;	break;
				default:
					log_printf (LOGT_ERROR, "Invalid tsproc type.\n");
					exit (1);
					break;
				}
				break;
			case ':':
				log_printf (LOGT_ERROR, "Missing mandatory parameter.\n");
				exit (1);
				break;
			case '?':
				log_printf (LOGT_ERROR, "Unknown parameter provided.\n");
				exit (1);
				break;
			default:
				log_printf (LOGT_ERROR, "Unrecognized option.\n");
				exit (1);
			break;
		}
	}

}


/* stream live media from dvr to file.
   a child process is opened which will talk to the DVR directly,
   while the parent will collect data from a pipe. */
/* container: 0-raw 1-DHAV 2-Matroska */
/* return ==0 ok, !=0 error (program should abort ASAP) */
//int process_dhav_stream (dvrcontrol_t *dvrctl, int media_container, const char *filename)
int process_dhav_stream (dvrcontrol_t *dvrctl, const char *in_file, const char *out_file)
{
	uint8_t sbuf_data[STREAM_IN_BUFFER_LEN];
	uint8_t sbuf_data_2[STREAM_OUT_BUFFER_LEN]; /* secondary buffer (not always necessary) */
	uint8_t *sbuf;
	uint8_t *sbuf_2;
	ssize_t sbuf_len;
	ssize_t sbuf_len_2;
	uint8_t *outbuf;	/* post-processing buffer (an alias, actually), may point to sbuf or sbuf_2 */
	size_t *outbuf_len_p;	/* similarly to outbuf, it is also an alias to sbuf_len or sbuf_len_2 */
	t_infile *infile;
	t_outfile *outfile;
	t_mc_parms *mc_parms;	/* media container parms */
	t_mc_format mc_format_in;	/* media container type */
	t_mc_format mc_format_out;	/* media container type */
	dstf_t *dstf;
	int dstf_ret;
	int dtconv_ret;
	int outfwrite_ret;
	int infread_ret;
	t_mc_tsproc *tsc;
	bool main_mkv_header_pending = true;

	sbuf = sbuf_data;
	sbuf_2 = sbuf_data_2;

	/* TODO: write this in a less kludgy way */
	if ((infile = infile_open (in_file)) == NULL) {
		log_printf (LOGT_FATAL, "Unable to read DHAV stream data.\n");
		return 4;
	}
	if ((outfile = outfile_open (out_file)) == NULL) {
		log_printf (LOGT_FATAL, "Unable to output MKV stream data.\n");
		infile_close (infile);
		return 4;
	}
	if ((dstf = dstf_init ()) == NULL) {
		log_printf (LOGT_FATAL, "Unable to allocate dstf.\n");
		infile_close (infile);
		outfile_close (outfile);
		return 7;
	}

	/* define mc_format (output file container, which is MKV) */
	mc_format_out = MC_FORM_MKV;
	outbuf = sbuf_2;
	outbuf_len_p = &sbuf_len_2;

	/* we initialize this later, to avoid allocation
	   just before several other things may fail */
	mc_parms = mc_init (mc_format_out, dvrctl->ntsc_exact_60hz);	/* FIXME: error should be checked */

	if (dvrctl->tsproc != TSPROC_NONE) {
		tsc = dt_tsproc_init (mc_parms, dvrctl->tsproc); /* FIXME: error should be checked */
	}

	/* collect data until there's enough to identify
	   the type of container */
	log_printf (LOGT_INFO, "Identifying type of media container in stream...\n");
	mc_format_in = MC_FORM_DVR_UNKNOWN;
	while ((dstf->sq_maxlen - dstf->sq_len) >= (2 * STREAM_IN_FREAD_GRANULARITY)) {
		infread_ret = sbuf_len = infile_read (infile, sbuf, STREAM_IN_FREAD_GRANULARITY);

		if (infread_ret <= 0) {
			log_printf (LOGT_INFO, "No more data to read.\n");
			break;
		}

		if (sbuf_len > 0) {
			/* append data into dstf */
			memcpy ((dstf->sq_p + dstf->sq_offs + dstf->sq_len), sbuf, sbuf_len);
			dstf->sq_len += sbuf_len;

			/* search for pattern in data stored in dstf */
			mc_format_in = identify_mc_format (dstf->sq_p + dstf->sq_offs, dstf->sq_len);
			if (mc_format_in != MC_FORM_DVR_UNKNOWN) {
				break;
			}
		}
	}
	if (mc_format_in == MC_FORM_DVR_UNKNOWN) {
		log_printf (LOGT_ERROR, "Unable to identify media container format "
			"from incoming stream. Software bug or unknown "
			"media container. PLEASE CONTACT THE DEVELOPER AND REPORT.\n");
		/* assume media container is DHAV, the most commonly found,
		   and hope for the best */
		mc_format_in = MC_FORM_DHAV;
	}


	while (1) {
		/* read-and-convert loop */

		/* WARNING: blocking IO here */
		infread_ret = sbuf_len = infile_read (infile, sbuf, STREAM_IN_FREAD_GRANULARITY);
		DEBUG_LOG_PRINTF ("read from input stream: %d bytes\n", sbuf_len);

		/* grab frames from stream, convert,
		   and send the resulting data */
		dstf_ret = dtconv_ret = outfwrite_ret = 0;
		do {
			/* note: this reads and writes back into the same buffer (sbuf) */
			if (mc_format_in == MC_FORM_DHAV) {
				/* MC_FORM_DHAV */
				dstf_ret = dstf_process_dhav_stream_to_frames (dstf, sbuf, (dstf_ret > 0) ? 0 : sbuf_len, sbuf, &sbuf_len, STREAM_IN_BUFFER_LEN);
			} else {
				/* MC_FORM_RAW_H264 */
				dstf_ret = dstf_process_raw_h264_stream_to_frames (dstf, sbuf, (dstf_ret > 0) ? 0 : sbuf_len, sbuf, &sbuf_len, STREAM_IN_BUFFER_LEN);
			}

			if (sbuf_len > 0) {
				/* there's a frame to process */

				/* convert DHAV/RAW_H264 data to MKV */
				if (mc_format_in == MC_FORM_DHAV) {
					/* MC_FORM_DHAV */
					dt_collect_dhav_frame_info (mc_parms, sbuf, sbuf_len);
				} else {
					/* MC_FORM_RAW_H264 */
					dt_collect_raw_h264_frame_info (mc_parms, sbuf, sbuf_len);
				}

				if ((dvrctl->tsproc != TSPROC_NONE) && (mc_format_in == MC_FORM_DHAV)) {
					dt_tsproc_process (tsc);
					mc_parms->v_timestamp = tsc->v_timestamp; /* override with fixed timestamp */
				}
				if (mc_format_in == MC_FORM_DHAV) {
					/* MC_FORM_DHAV */
					dtconv_ret = dt_convert_frame_to_mkv (mc_parms, sbuf, sbuf_len, sbuf_2, &sbuf_len_2, STREAM_OUT_BUFFER_LEN, main_mkv_header_pending, MCODEC_V_MPEG4_ISO_AVC);
				} else {
					/* MC_FORM_RAW_H264 */
					dtconv_ret = dt_convert_frame_to_mkv (mc_parms, sbuf, sbuf_len, sbuf_2, &sbuf_len_2, STREAM_OUT_BUFFER_LEN, main_mkv_header_pending, MCODEC_V_MPEG4_ISO_ASP);
				}
				if (dtconv_ret == 0)
					main_mkv_header_pending = false;

				/* WARNING: blocking IO here */
				if ((outfwrite_ret = outfile_write (outfile, outbuf, *outbuf_len_p)) != 0)
					break;
			}
		} while (dstf_ret > 0);

		if (dstf_ret < 0) {
			log_printf (LOGT_FATAL, "dstf_process_(dhav|h_264)_stream_to_frames failure: %d.\n", dstf_ret);
			break;
		}
		if (dtconv_ret < 0) {
			log_printf (LOGT_FATAL, "dt_convert_frame_to_mkv failure: %d.\n", dtconv_ret);
			break;
		}
		if (infread_ret <= 0) {
			log_printf (LOGT_INFO, "No more data to read.\n");
			break;
		}
		if (outfwrite_ret != 0) {
			log_printf (LOGT_FATAL, "Unable to write to target: %d.\n", outfwrite_ret);
			break;
		}

	}

	dstf_close (dstf);
	infile_close (infile);
	outfile_close (outfile);
	if (dvrctl->tsproc != TSPROC_NONE) {
		dt_tsproc_close (tsc);
	}
	mc_close (mc_parms);
	return 0;
}





int main (int argc, char **argv, char *env[])
{
	dvrcontrol_t dvrctl;

	log_define_context ("dhav2mkv");

	process_command_line_arguments (argc, argv);

	if (command_options.dvr_channel > 255) {
		log_printf (LOGT_ERROR, "Invalid channel specified: %d\n", dvrctl.channel);
		exit (0);
	}

	dvrctl.ntsc_exact_60hz = command_options.ntsc_exact_60hz;
	dvrctl.tsproc = command_options.tsproc;

	process_dhav_stream (&dvrctl, command_options.in_file, command_options.out_file);

	exit (0);
}

