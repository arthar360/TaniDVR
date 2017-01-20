/* main code */

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

#include "hlprotocol.h"
#include "log.h"
#include "devinfo.h"
#include "filetools.h"
#include "mctools.h"
#include "dvrcontrol.h"
#include "shtools.h"
#include "config.h"	/* autotools-generated */

struct struct_command_options {
	int operation_mode;
	const char *dvr_target;
	unsigned short int dvr_port;
	const char *dvr_user;
	const char *dvr_password;
	int dvr_channel;
	int dvr_sub_channel;
	int media_container;
	const char *out_file;
	unsigned int keep_alive;	/* user input in ms, later converted to us (x1000) */
	unsigned int timeout;		/* inactivity timeout for considering DVR connection dead */
	unsigned int net_protocol_dialect;
	bool ntsc_exact_60hz;
	tsproc_t tsproc;
} command_options;

void dump_device_information (t_devinfo *devinfo)
{
	printf ("Video system:\t");
	switch (devinfo->video_sys) {
	case DI_VS_UNDEFINED:	printf ("(undefined)");	break;
	case DI_VS_NTSC:	printf ("NTSC");	break;
	case DI_VS_PAL:		printf ("PAL");		break;
	case DI_VS_UNKNOWN:	printf ("(unknown)");	break;
	}
	printf ("\n");

	printf ("Video encoder:\t");
	switch (devinfo->video_enc) {
	case DI_VE_UNDEFINED:	printf ("(undefined)");	break;
	case DI_VE_MPEG4:	printf ("MPEG4");	break;
	case DI_VE_H264:	printf ("H.264");	break;
	case DI_VE_UNKNOWN:	printf ("(unknown)");	break;
	}
	printf ("\n");

	printf ("Multi-window:\t");
	switch (devinfo->mw_preview) {
	case DI_MWP_UNDEFINED:	printf ("(undefined)");	break;
	case DI_MWP_YES:	printf ("YES");		break;
	case DI_MWP_NO:		printf ("NO");		break;
	case DI_MWP_UNKNOWN:	printf ("(unknown)");	break;
	}
	printf ("\n");

	printf ("Total channels:\t");
	if (devinfo->n_channels < 0) {
		printf ("(undefined)");
	} else {
		printf ("%d", devinfo->n_channels);
	}
	printf ("\n");

	printf ("Device type:\t");
	if (devinfo->dev_type < 0) {
		printf ("(undefined)");
	} else {
		printf ("%d", devinfo->dev_type);
	}
	printf ("\n");

	printf ("Device subtype:\t");
	if (devinfo->dev_subtype < 0) {
		printf ("(undefined)");
	} else {
		printf ("%d", devinfo->dev_subtype);
	}
	printf ("\n");

	printf ("Chipset/model:\t%s\n", devinfo->dev_type_chipset);

	printf ("HD type:\t");
	switch (devinfo->hd_type) {
	case DI_HD_UNDEFINED:	printf ("(undefined)"); break;
	case DI_HD_ATA:		printf ("ATA");		break;
	case DI_HD_SATA:	printf ("SATA");	break;
	case DI_HD_UNKNOWN:	printf ("(unknown)");	break;
	}
	printf ("\n");

	printf ("Device S/N:\t%s\n", devinfo->dev_sn);
	printf ("Device version:\t%s\n", devinfo->dev_ver);

	printf ("Lang. support:\t%s\n", devinfo->lang_sup);
	printf ("Function list:\t%s\n", devinfo->func_list);
	printf ("OEM info:\t%s\n", devinfo->oem_info);
	printf ("Running status:\t%s\n", devinfo->netrun_stat);
	printf ("Split screen:\t%s\n", devinfo->split_screen);
	printf ("Func. capab.:\t%s\n", devinfo->func_capability);
	printf ("HD part. cap.:\t%s\n", devinfo->hd_part_cap);
	printf ("Wireless alarm:\t%s\n", devinfo->wi_alarm);
	printf ("Access URL:\t%s\n", devinfo->access_url);

	printf ("\n");
}

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
		{"operation-mode", 1, 0, 'm'},
		{"dvr-target", 1, 0, 't'},
		{"dvr-port", 1, 0, 'p'},
		{"dvr-user", 1, 0, 'u'},
		{"dvr-passwd", 1, 0, 'w'},
		{"dvr-channel", 1, 0, 'c'},
		{"dvr-sub-channel", 1, 0, 's'},
		{"media-container", 1, 0, 'n'},
		{"out-file", 1, 0, 'f'},
		{"keep-alive", 1, 0, 'k'},
		{"timeout", 1, 0, 'e'},
		{"sixty-hertz-ntsc", 0, 0, 'x'},
		{"ts-proc", 1, 0, 'r'},
		{"net-protocol-dialect", 1, 0, 'a'},
		{0, 0, 0, 0}
	};

	command_options.dvr_port = 37777;
	command_options.dvr_channel = 0;
	command_options.dvr_sub_channel = 0;
	command_options.media_container = 1;
	command_options.out_file = "\0"; /* empty = stdout */
	command_options.keep_alive = 100;
	command_options.timeout = 5000;
	command_options.net_protocol_dialect = 0;
	command_options.ntsc_exact_60hz = false;
	command_options.tsproc = TSPROC_DO_CORRECT;

	while ((option = getopt_long (argc, argv, "a:hm:t:p:u:w:c:s:n:f:k:e:xr:", long_options, &option_index)) != EOF) {
		switch (option) {
			case 'h':
				printf ("TaniDVR " VERSION "\n"
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
						
						"Usage: tanidvr <-m <mode>> <-t <address>> <-u <user>> <-w <password>> [-p <port>] [-c <channel>] [-s <sub-channel>] [-n <container ID>] [-f <filename>] [(...)] [-h]\n\n"
						"-m, --operation-mode\n"
							"\t0 - display DVR information\n"
							"\t1 - dump video\n"
							"\n"
						"-t, --dvr-target\n\tIP/hostname\n\n"
						"-p, --dvr-port\n\tNetwork port (default 37777)\n\n"
						"-u, --dvr-user\n\tDVR user (may not work if not admin!)\n\n"
						"-w, --dvr-password\n\tDVR password\n\n"
						"-c, --dvr-channel\n\t0-255 (default 0)\n\n"
						"-s, --dvr-sub-channel\n"
							"\t0 - main (default)\n"
							"\t1 - secondary\n"
							"\n"
						"-a, --net-protocol-dialect\n"
							"\t0 - common dialect (default)\n"
							"\t1 - common dialect, while emulating OEM client behavior\n"
							"\n"
						"-n, --media-container\n"
							"\t0 - DVR native: DHAV (.dav|.dhav) or RAW H.264 (depends on the DVR itself)\n"
							"\t1 - Matroska (.mkv) (default)\n"
							"\n"
						"-f, --out-file\n\t<filename> (default: empty -- console stdout)\n\n"
						"-k, --keep-alive\n\t<mili_seconds> (default: 100ms)\n"
							"\tSend innocuous packets to the DVR in order to avoid the\n"
							"\tconnection to be dropped gratuitously.\n"
							"\tTo disable (not recommended), set to 0.\n\n"
						"-e, --timeout\n\t<mili_seconds> (default 5000ms)\n"
							"\tInactivity timeout for DVR connection to be considered dead.\n"
							"\tA new DVR connection is started after this.\n"
							"\tTo disable (not recommended), set to 0.\n\n"
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
			case 'a':
				sscanf (optarg, "%d", &p);
				if ((p <= 0) || (p > 1)) {
					log_printf (LOGT_ERROR, "Invalid network protocol dialect.\n");
					exit (1);
				}
				command_options.net_protocol_dialect = p;
				break;
			case 'm':
				defined_operation_mode = true;
				sscanf (optarg, "%d", &(command_options.operation_mode));
				switch (command_options.operation_mode) {
				case 0:
				case 1:
					break;
				default:
					log_printf (LOGT_ERROR, "Invalid operation mode.\n");
					exit (1);
					break;
				}
				break;
			case 't':
				defined_dvr_target = true;
				command_options.dvr_target = optarg;
				break;
			case 'p':
				sscanf (optarg, "%d", &p);
				if ((p <= 0) || (p > 65535)) {
					log_printf (LOGT_ERROR, "Invalid port number.\n");
					exit (1);
				}
				command_options.dvr_port = p;
				break;
			case 'u':
				defined_dvr_user = true;
				command_options.dvr_user = optarg;
				break;
			case 'w':
				defined_dvr_password = true;
				command_options.dvr_password = optarg;
				break;
			case 'c':
				sscanf (optarg, "%d", &(command_options.dvr_channel));
				if ((command_options.dvr_channel < 0) || (command_options.dvr_channel > 255)) {
					log_printf (LOGT_ERROR, "Invalid DVR channel.\n");
					exit (1);
				}
				break;
			case 's':
				sscanf (optarg, "%d", &(command_options.dvr_sub_channel));
				if ((command_options.dvr_sub_channel < 0) || (command_options.dvr_sub_channel > 1)) {
					log_printf (LOGT_ERROR, "Invalid DVR sub-channel.\n");
					exit (1);
				}
				break;
			case 'n':
				sscanf (optarg, "%d", &(command_options.media_container));
				switch (command_options.media_container) {
				case 0:
				case 1:
					break;
				default:
					log_printf (LOGT_ERROR, "Invalid media container.\n");
					exit (1);
					break;
				}
				break;
			case 'f':
				command_options.out_file = optarg;
				break;
			case 'k':
				sscanf (optarg, "%d", &p);
				if ((p > 1000000) || (p < 0)) {
					log_printf (LOGT_ERROR, "Out-of-range keep-alive value.\n");
					exit (1);
				}
				command_options.keep_alive = p;
				break;
			case 'e':
				sscanf (optarg, "%d", &p);
				if ((p > 1000000) || (p < 0)) {
					log_printf (LOGT_ERROR, "Out-of-range timeout value.\n");
					exit (1);
				}
				command_options.timeout = p;
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

	if ((command_options.keep_alive != 0) && \
		(command_options.timeout != 0) &&\
		(command_options.keep_alive > command_options.timeout)) {

		log_printf (LOGT_ERROR, "Keep-alive must be either shorter than Timeout, or one of those must be disabled.\n");
	}

	if (defined_operation_mode == false) {
		log_printf (LOGT_ERROR, "It is required to define an operation mode.\n");
		exit (1);
	}
	if (defined_dvr_target == false) {
		log_printf (LOGT_ERROR, "It is required to define a DVR target.\n");
		exit (1);
	}
	if (defined_dvr_user == false) {
		log_printf (LOGT_ERROR, "It is required to define a DVR user.\n");
		exit (1);
	}
	if (defined_dvr_password == false) {
		log_printf (LOGT_ERROR, "It is required to define a DVR password.\n");
		exit (1);
	}
	/* NOTE: the following depends on defined_dvr_user and defined_dvr_password both being TRUE */
	if ((strlen (command_options.dvr_user) + strlen (command_options.dvr_password)) >  MAX_USER_PASSWD_LEN) {
		log_printf (LOGT_ERROR, "The maximum allowed total size for both \"user\" and \"password\" strings is %d characters.\n", MAX_USER_PASSWD_LEN);
		exit (1);
	}

}



int main (int argc, char **argv, char *env[])
{
	t_hlp_connection conn_control;
	t_hlp_connection conn_stream;
	t_devinfo devinfo;
	int i, j;
	char *channel_names;
	dvrcontrol_t dvrctl;

	/* initialize signal handlers */
	sht_init (SHT_BASE_PROC);

	/* set logging context for this process */
	log_define_context ("main");

	process_command_line_arguments (argc, argv);

	dvrctl.hostname = command_options.dvr_target;
	dvrctl.port = command_options.dvr_port;
	dvrctl.user = command_options.dvr_user;
	dvrctl.passwd = command_options.dvr_password;
	dvrctl.channel = command_options.dvr_channel;
	dvrctl.sub_channel = command_options.dvr_sub_channel;
	dvrctl.keep_alive_us = command_options.keep_alive * 1000;	/* this one in micro-seconds */
	dvrctl.timeout_us = command_options.timeout * 1000;		/* this one in micro-seconds */
	dvrctl.ntsc_exact_60hz = command_options.ntsc_exact_60hz;
	dvrctl.tsproc = command_options.tsproc;
	dvrctl.net_protocol_dialect = command_options.net_protocol_dialect;

	switch (command_options.operation_mode) {
	case 0:
		/* show all the device information we've collected so far */
		di_init (&devinfo);
		if (open_session (&conn_control, &devinfo, &dvrctl) != 0) {
			exit (1);
		}
		dump_device_information (&devinfo);
		if (close_session (&conn_control) != 0) {
			exit (1);
		}
		break;
	case 1:
		/* get device information */
		di_init (&devinfo);
		if (open_session (&conn_control, &devinfo, &dvrctl) != 0) {
			exit (1);
		}
		close_session (&conn_control);	// ignore logout fail, some DVRs always fail at this

		/* check if channel is valid */
		if ((devinfo.n_channels > 0) && (dvrctl.channel >= devinfo.n_channels)) {
			log_printf (LOGT_ERROR,
				"Invalid channel specified. This device supports channels 0 to %d.\n",
				devinfo.n_channels - 1);
			exit (1);
		}

		/* stream video */
		stream_media_dvr_to_file (&dvrctl, command_options.media_container, command_options.out_file);
		break;
	}

	exit (0);
}

