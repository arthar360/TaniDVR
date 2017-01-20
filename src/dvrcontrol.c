/* dvrcontrol.c */

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

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <poll.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include "hlprotocol.h"
#include "devinfo.h"
#include "log.h"
#include "mctools.h"
#include "mptools.h"
#include "filetools.h"
#include "timertools.h"
#include "dvrcontrol.h"
#include "bufftools.h"
#include "shtools.h"

#define STREAM_BUFFER_LEN 1000000
#if (STREAM_BUFFER_LEN * 4) > SSIZE_MAX
#define STREAM_BUFFER_MAXPIPEREAD SSIZE_MAX
#else
#define STREAM_BUFFER_MAXPIPEREAD (STREAM_BUFFER_LEN / 4)
#endif


#ifdef DEBUG

#define SYSINFO(code,bstr) i = hlp_get_system_information (conn_control, devinfo, code, bstr); \
	DEBUG_LOG_PRINTF ("hlp_get_system_information %d returned: %d\n", code, i);
#define MEDIACAP(code) i = hlp_get_media_capability (conn_control, code); \
	DEBUG_LOG_PRINTF ("hlp_get_media_capability %d returned: %d\n", code, i);
#define CONFPARM(code) i = hlp_get_config_parameter (conn_control, code); \
	DEBUG_LOG_PRINTF ("hlp_get_config_parameter %d returned: %d\n", code, i);
#define SETALARM(code) i = hlp_set_alarm (conn_control, code); \
	DEBUG_LOG_PRINTF ("hlp_set_alarm %d returned: %d\n", code, i);

#else

#define SYSINFO(code,bstr) i = hlp_get_system_information (conn_control, devinfo, code, bstr);
#define MEDIACAP(code) i = hlp_get_media_capability (conn_control, code);
#define CONFPARM(code) i = hlp_get_config_parameter (conn_control, code);
#define SETALARM(code) i = hlp_set_alarm (conn_control, code);

#endif

/* returns:
   <0, unrecoverable error
    (-1, poll error ; -2, fd error)
    0, poll timeout or interruption
   >0 data in */
static int poll_pipe_read (int fd, int timeout)
{
	struct pollfd waitfd;
	int retpoll;

	waitfd.fd = fd;
	waitfd.events = POLLIN | POLLPRI;

	retpoll = poll (&waitfd, 1, timeout);
	if ((retpoll == 0) || ((retpoll == -1) && (errno == EINTR))) {
		return 0;
	} else if (retpoll == -1) {
		return -1;
	}
	return (((waitfd.revents) & (POLLIN | POLLPRI)) ? 1 : -2);
}

/* returns ==0 ok, !=0 error (program should abort ASAP) */
int open_session (t_hlp_connection *conn_control, t_devinfo *devinfo, dvrcontrol_t *dvrctl)
{
	int i, j;

	i = hlp_open (conn_control, dvrctl->hostname, dvrctl->port, dvrctl->timeout_us);
	if (i != 0) {
		log_printf (LOGT_ERROR, "Unable to open connection to DVR.\n");
		return 1;
	}

	i = hlp_login (conn_control, devinfo, dvrctl->user, dvrctl->passwd);
	if (i != 0) {
		log_printf (LOGT_ERROR, "Login failed.\n");
		return 1;
	}

	if (dvrctl->net_protocol_dialect == 0) {
		/* 0: common protocol dialect.
		      this does only what is strictly needed, avoiding the
		      excessive, unnecessary, chatting behavior done by
		      the OEM client. */

		/* get all the information we can */
		i = hlp_get_system_information_multiple (conn_control, devinfo, 0, 256);

	} else {
		/* 1: common protocol dialect, and emulates OEM client behavior.
		      some DVRs may require this exact chatting sequence in order
		      to work, shutting themselves up if not done exactly as the
		      OEM client does. */

		i = hlp_get_work_alarm_status (conn_control);
		DEBUG_LOG_PRINTF ("hlp_get_work_alarm_status returned: %d\n", i);

		SYSINFO(1,0);
		SYSINFO(7,0);
		SYSINFO(2,0);

		/*
		i = hlp_get_channel_names (&conn_control, &channel_names);
		DEBUG_LOG_PRINTF ("hlp_get_channel_names returned: %d\n", i);
		if (i == 0) {
			printf ("channel names result:\n%s\n", channel_names);
			free (channel_names);
		}
		*/

		MEDIACAP(0);
		MEDIACAP(1);

		CONFPARM(0x10);

		SYSINFO(0x20,0);
		SYSINFO(0x04,0xff);

		/* send extension string */
		/* ??? but the DVR expects that */
		hlp_send_extension_string (conn_control,
			"TransactionID:1\r\n"
			"Method:GetParameterNames\r\n"
			"ParameterName:Dahua.Device.VideoOut.General\r\n"
			"\r\n");
		DEBUG_LOG_PRINTF ("hlp_send_extension_string returned: %d\n", i);

		SYSINFO(0x0a,0);
		SYSINFO(2,0);

		SETALARM(0x01);
		// bulk
		SETALARM(0x02);
		SETALARM(0x03);
		SETALARM(0x04);
		SETALARM(0x05);
		SETALARM(0x06);
		SETALARM(0x07);
		SETALARM(0x08);
		SETALARM(0x09);
		SETALARM(0x0a);
		SETALARM(0x0b);
		SETALARM(0x0c);
		SETALARM(0x0d);
		SETALARM(0x0e);
		SETALARM(0x0f);
		SETALARM(0x10);
		SETALARM(0x12);
		SETALARM(0x13);
		SETALARM(0x14);
		SETALARM(0x9c);
		SETALARM(0xa1);
		SETALARM(0xa2);

		/* get all the information we can */
		for (j = 0; j < 40; j++) {
			SYSINFO(j,0);
		}

		i = hlp_get_work_alarm_status (conn_control);
		DEBUG_LOG_PRINTF ("hlp_get_work_alarm_status returned: %d\n", i);
	}

	return 0;
}

/* returns ==0 ok, !=0 error (program should abort ASAP) */
int close_session (t_hlp_connection *conn_control)
{
	int i;

	i = hlp_logout (conn_control);
	if (i != 0) {
		log_printf (LOGT_ERROR, "Logout failed.\n");
		return 1;
	}

	hlp_close (conn_control);

	return 0;
}

/*  ********************** USED BY GRANDCHILD PROCESS */
/* stream live media from DVR to pipe (child process -> parent)
   connection to DVR is started from scratch.
   ppkf is assumed to be properly initialized.
   -- this is an auxiliary function intended to be called from a child process,
      invoked by other functions such as stream_media_dvr_to_file() */
/* return ==0 ok, !=0 error (< 100, recoverable somehow ; >= 100 internal error, program should abort ASAP) */
int stream_media_dvr_to_pipe (mptools_pipedfork_t *ppfk, dvrcontrol_t *dvrctl)
{
	t_hlp_connection conn_control_r;
	t_hlp_connection *conn_control;
	t_hlp_connection conn_stream_r;
	t_hlp_connection *conn_stream;
	uint8_t sbuf_data[STREAM_BUFFER_LEN];
	uint8_t *sbuf;
	ssize_t sbuf_len;
	t_hlp_connection *hlp_connection[2];
	t_devinfo devinfo;
	int loopret = 0;	/* in-loop error code */
	spenttime_t ka_timer;	/* keep-alive (differential) timer, avoid messing with SIGALRM */
	spenttime_t to_timer;	/* timeout (differential) time, avoid messing with SIGALRM */
	unsigned int timeout_hlp_wait = (dvrctl->keep_alive_us != 0) ? (dvrctl->keep_alive_us / 1000) : 100;	/* if keepalive, timeout=keepalive; otherwise timeout = 100ms */

	/* just for the sake of consistency */
	conn_control = &conn_control_r;
	conn_stream = &conn_stream_r;

	sbuf = sbuf_data;

	/* start CONTROL connection */
	di_init (&devinfo);
	DEBUG_LOG_PRINTF ("start CONTROL connection\n");
	if (open_session (conn_control, &devinfo, dvrctl) != 0) {
		log_printf (LOGT_ERROR, "Failed to stablish DVR network session.\n");
		return 4;
	}

	/* start STREAM connection */
	DEBUG_LOG_PRINTF ("start STREAM connection...\n");
	if (hlp_open (conn_stream, dvrctl->hostname, dvrctl->port, dvrctl->timeout_us) != 0) {
		log_printf (LOGT_ERROR, "Unable to open stream connection.\n");
		return 1;
	}

	/* tie conn_stream to conn_control */
	DEBUG_LOG_PRINTF ("tie conn_stream to conn_control...\n");
	if (hlp_connection_relationship (conn_control, conn_stream, 1, dvrctl->channel) != 0) {
		log_printf (LOGT_ERROR, "Unable to establish a connection relationship between control and stream channels.\n");
		return 2;
	}

	/* first frame... */
	DEBUG_LOG_PRINTF ("first frame...\n");
	if (hlp_media_data_request (conn_control, conn_stream, dvrctl->channel, dvrctl->sub_channel, sbuf, STREAM_BUFFER_LEN, &sbuf_len) != 0) {
		log_printf (LOGT_ERROR, "Error while requesting media data.\n");
		return 3;
	}
	if (write (ppfk->fd_write, sbuf, sbuf_len) == -1) {
		log_printf (LOGT_FATAL, "Cannot write to pipe.\n");
		return 105;
	}


	/* second frame and the rest... */

	hlp_connection[0] = conn_control;
	hlp_connection[1] = conn_stream;

	spenttime_set (&ka_timer);	/* set a starting time for keep-alive timer */
	spenttime_set (&to_timer);	/* set a starting time for timeout timer */

	while (1) {
		/* wait up to timeout_hlp_wait for data, return regardless */
		if (hlp_wait_for_incoming_data (&hlp_connection[0], 2, timeout_hlp_wait) != 0) {
			while (hlp_check_incoming_data (conn_stream) > 0) {
				spenttime_set (&to_timer);	/* reset DVR timeout */

				if (hlp_collect_media_data (conn_stream, 0, sbuf, STREAM_BUFFER_LEN, &sbuf_len) != 0) {
					log_printf (LOGT_ERROR, "DVR/network error (hlp_collect_media_data).\n");
					loopret = 6;
					break;
				}

				/* FIXME: is that a good idea to keep this blocking? */
				if (write (ppfk->fd_write, sbuf, sbuf_len) == -1) {
					log_printf (LOGT_DETAIL, "Cannot write to pipe.\n");
					loopret = 105;
					break;
				}
				if (dvrctl->keep_alive_us != 0) {
					if (spenttime_get (&ka_timer) > dvrctl->keep_alive_us) {
						break;	/* keep-alive timeout, leave the rest for later... */
					}
				}
			}
			if (loopret != 0) {
				break;	/* error from previous inner loop, avoid extra useless processing */
			}

			if (hlp_discard_incoming_data (conn_control) < 0) {
				log_printf (LOGT_ERROR, "DVR/network error (hlp_discard_incoming_data).\n");
				loopret = 7;
				break;
			}
		}

		if (dvrctl->keep_alive_us != 0) {
			/* the DVR firmware has the bad habit of dropping connections
			   too easily. this forces the DVR to keep the connection up. */
			if (spenttime_get (&ka_timer) > dvrctl->keep_alive_us) {
				/* keep-alives the control channel (the DVR likes to drop connections otherwise)
				   and check whether the connection is alive still */
				if (hlp_keep_alive (conn_control) != 0) {
					log_printf (LOGT_ERROR, "DVR/network error (hlp_keep_alive).\n");
					loopret = 8;
					break;
				}
				spenttime_set (&ka_timer);	/* reset keep-alive timer */
			}
		}

		if (dvrctl->timeout_us != 0) {
			if (spenttime_get (&to_timer) > dvrctl->timeout_us) {
				/* DVR timeout, give up */
				log_printf (LOGT_WARNING, "DVR session timeout.\n");
				loopret = 9;
				break;
			}
		}
	}

	hlp_close (conn_stream);
	hlp_close (conn_control);
	return loopret;
}

/* set/unset O_NONBLOCK fd flag:
   true=blocking; false=O_NONBLOCK set.
   returns: ==0 ok, !=0 error */
int set_fd_blocking (int fd, bool doesblock)
{
	int fd_flags;

	fd_flags = fcntl (fd, F_GETFL, 0);
	if (doesblock == true) {
		return (fcntl (fd, F_SETFL, fd_flags & ~O_NONBLOCK));
	}
	return (fcntl (fd, F_SETFL, fd_flags | O_NONBLOCK));
}

/*
 * Transfers continuously data from fd_in to fd_out,
 * until somethings goes wrong or fd_in stops sending data longer than timeout_us.
 *
 * PARAMETERS:
 * 	- requires initialized btfifo.
 * 	- requires opened fd_in and fd_out with O_NONBLOCK successfully set.
 *	- timeout_us: micro-seconds or 0 to disable.
 *	- ppfk_child to monitor if that process is still alive in case
 *        timeout_us==0 ; if NULL, does not check regardless.
 * RETURNS:
 * 	>0 recoverable condition (eg. timeout)
 * 	<0 fatal error
 * 	==0 (not used)
 * 
 * Currently used by stream_dvr_subprocess_to_pipe(), only.
 */
int buffered_tunnel_pipe (btfifo_t *btfifo, int fd_in, int fd_out, unsigned int timeout_us)
{
	int retval = 0;
	struct pollfd p[2];
	struct pollfd *p_in;
	struct pollfd *p_out;
	ssize_t retp;
	bool has_timeout;
	int poll_timeout;

	p_in = &(p[0]);
	p_out = &(p[1]);
	has_timeout = (timeout_us != 0) ? true : false;
	poll_timeout = (has_timeout == true) ? (timeout_us / 1000) : 1000;	/* if timeout_us is disabled (0), uses 1000ms */

	p_in->fd = fd_in;
	p_out->fd = fd_out;

	while (1) {
		p_in->events = (btfifo_get_free_len (btfifo) != 0) ? (POLLIN | POLLPRI) : 0;
		p_out->events = (btfifo_get_stored_len (btfifo) != 0) ? (POLLOUT) : 0;
		retp = poll (p, 2, poll_timeout);
		if ((retp == -1) && (errno != EINTR)) {
			log_printf (LOGT_DETAIL, "Poll error.\n");
			return 2;
		} else if (retp == 0) {
			if (has_timeout == true) {
				log_printf (LOGT_WARNING, "Timeout.\n");
				return 1;
			} else if (sht_fl_sigchld != 0) {
				/* FIXME: sht_fl_sigchld is not cleared safely (signal-wise),
				   though it works now since (total children <= 1) always. */
				log_printf (LOGT_DETAIL, "Child died.\n");
				sht_fl_sigchld = 0;
				return 6;
			}
		}

		/* FIFO -> fd_out */
		/* keep this first to minimize FIFO usage */
		if ((p_out->revents) & (POLLOUT)) {
			if ((retp = btfifo_fifo_to_fd (btfifo, fd_out)) <= -100) {
				log_printf (LOGT_DETAIL, "FIFO to FD: %d.\n", (int) retp);
				return 4;
			}
		} else if ((p_out->revents) & (POLLERR | POLLHUP | POLLNVAL)) {
			log_printf (LOGT_DETAIL, "Write socket closed remotely.\n");
			return -5;
		}

		/* fd_in -> FIFO */
		if ((p_in->revents) & (POLLIN | POLLPRI)) {
			if ((retp = btfifo_fd_to_fifo (btfifo, fd_in)) <= -100) {
				log_printf (LOGT_DETAIL, "FD to FIFO: %d.\n", (int) retp);
				return 3;
			}
		} else if ((p_in->revents) & (POLLERR | POLLHUP | POLLNVAL)) {
			log_printf (LOGT_DETAIL, "Read socket closed remotely.\n");
			return 10;
		}
	}

	return 0;	/* not expected to reach this point */
}

/* used by intermediate process between root parent process and the DVR control
   process (the latter calling stream_media_dvr_to_pipe() ).
   it is done so in order to enforce timeouts immediately and allow the fast
   restablishment of a new connection to DVR.

   this function should return ONLY when there is an unrecoverable error
   (this function's child is the one to be created over and over. */
/* returns:
	>=100 unrecoverable error/situation (parent broke pipe,
		failure of mandatory functions such as malloc()/etc and other)
	<100 (including 0 and negative values) are not expected to be returned. */
int stream_dvr_subprocess_to_pipe (mptools_pipedfork_t *ppfk_parent, dvrcontrol_t *dvrctl)
{
	mptools_pipedfork_t ppfk_child_r;
	mptools_pipedfork_t *ppfk_child;
	pid_t ppfk_child_ret;
	int child_ret;
	int rettp;
	int retcode = 0;
	bool delay_start = false;
	//
	btfifo_t btfifo_r;
	btfifo_t *btfifo;
	int fd_in;
	int fd_out;

	ppfk_child = &ppfk_child_r;
	btfifo = &btfifo_r;

	btfifo->bsize = 1048576;	/* FIXME - this should not be hardcoded */
	btfifo->flags = BTFIFO_F_ASSUME_FD_READY;
	//btfifo->wmin = 0;
	if (btfifo_create (btfifo) != 0)
		return 101;

	while (1) {
		log_printf (LOGT_INFO, "Starting DVR session controller...\n");

		/* start a child process */
		/* block signals and fork */
		sht_signalblock_mgr (SHT_OP_BLOCK, (SHT_F_SIGBLOCK_SIGSTD));
		ppfk_child_ret = mptools_create_pipedfork (ppfk_child);
		if (ppfk_child_ret == 0) {
			/* child */

			/* set logging context for this process */
			log_define_context ("DVR streamer");

			/* initialize signal handlers for child and unblock signals */
			sht_init (SHT_DVRCTL_PROC);
			sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGSTD));

			mptools_child_close_pipedfork (ppfk_parent); /* close grandparent's pipe */
			btfifo_destroy (btfifo); /* useless here, avoid post-fork() buffer duplication */

			log_printf (LOGT_INFO, "DVR session controller started.\n");

			child_ret = stream_media_dvr_to_pipe (ppfk_child, dvrctl);
			if (child_ret >= 100) {
				log_printf (LOGT_DETAIL, "stream_media_dvr_to_pipe() returned unrecoverable error.\n");
			} else if (child_ret != 0) {
				log_printf (LOGT_ERROR, "DVR dropped connection or general network error.\n");
			} else {
				log_printf (LOGT_DETAIL, "stream_media_dvr_to_pipe() returned 0\n");
			}

			mptools_child_close_pipedfork (ppfk_child);
			_exit (child_ret);	/* child gone */
		}

		/* parent (regardless fork success) */

		/* safe to unblock signals now */
		sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGSTD));

		if (ppfk_child_ret == -1) {
			log_printf (LOGT_FATAL, "Pipedfork failed.\n");
			retcode = 100;
			break;
		}

		/* if the DVR process starts failing too fast,
		   this avoids a messy busy-loop-like situation for
		   localhost, DVR (which may crash) and network. */
		if (delay_start == true)
			sleep (1);

		log_printf (LOGT_DETAIL, "DVR session controller started. Waiting for data from that...\n");

		fd_in = ppfk_child->fd_read;
		fd_out = ppfk_parent->fd_write;

		/* btfifo REQUIRES fd set to O_NONBLOCK */
		/* FIXME - the return value should be checked */
		set_fd_blocking (fd_in, false);
		set_fd_blocking (fd_out, false);

		/* act as an intermediate and transfer data.
		   grandchild -> base parent */
		if ((rettp = buffered_tunnel_pipe (btfifo, fd_in, fd_out, dvrctl->timeout_us)) < 0) {
			log_printf (LOGT_DETAIL, "buffered_tunnel_pipe() returned: %d\n", rettp);
			retcode = 102;
		}

		/* dead/unresposive child.
		   finish it and its related resources */
		set_fd_blocking (fd_in, true);
		set_fd_blocking (fd_out, true);
		mptools_destroy_pipedfork (ppfk_child);

		if (retcode != 0)
			break;	/* fatal error, this process is gone too */

		log_printf (LOGT_ERROR, "DVR session controller quit or crashed.\n");
		log_printf (LOGT_INFO, "Attempting to recreate DVR session...\n");
		delay_start = true;
	}

	/* fatal error, close and return */
	set_fd_blocking (fd_in, true);
	set_fd_blocking (fd_out, true);
	btfifo->flags = (btfifo->flags & (~BTFIFO_F_USE_WMIN));
	btfifo_fifo_to_fd (btfifo, fd_out);	/* flushes fifo */
	btfifo_destroy (btfifo);

	return retcode;
}

/* ---------------------------- */

/* stream live media from dvr to file.
   a child process is opened which will talk to the DVR directly,
   while the parent will collect data from a pipe. */
/* container: 0-raw 1-DHAV 2-Matroska */
/* return ==0 ok, !=0 error (program should abort ASAP) */
int stream_media_dvr_to_file (dvrcontrol_t *dvrctl, int media_container_out, const char *filename)
{
	uint8_t sbuf_data[STREAM_BUFFER_LEN];
	uint8_t sbuf_data_2[STREAM_BUFFER_LEN]; /* secondary buffer (not always necessary) */
	uint8_t *sbuf;
	uint8_t *sbuf_2;
	ssize_t sbuf_len;
	ssize_t sbuf_len_2;
	uint8_t *outbuf;	/* post-processing buffer (an alias, actually), may point to sbuf or sbuf_2 */
	size_t *outbuf_len_p;	/* similarly to outbuf, it is also an alias to sbuf_len or sbuf_len_2 */
	t_outfile *outfile;
	t_mc_parms *mc_parms;		/* media container parms */
	t_mc_format mc_format_out;	/* media container type - output */
	t_mc_format mc_format_in;	/* media container type - input from DVR */
	mptools_pipedfork_t ppfk_r;
	mptools_pipedfork_t *ppfk;
	pid_t ppfk_ret;
	int child_ret;
	dstf_t *dstf;
	int dstf_ret;
	int dtconv_ret;
	int outfwrite_ret;
	t_mc_tsproc *tsc;
	bool main_mkv_header_pending = true;

	ppfk = &ppfk_r;
	sbuf = sbuf_data;
	sbuf_2 = sbuf_data_2;

	/* block signals and fork */
	sht_signalblock_mgr (SHT_OP_BLOCK, (SHT_F_SIGBLOCK_SIGSTD));
	ppfk_ret = mptools_create_pipedfork (ppfk);
	if (ppfk_ret == 0) {
		/* child */

		/* set logging context for this process */
		log_define_context ("buffer");

		/* initialize signal handlers for child and unblock signals */
		sht_init (SHT_BUFFER_PROC);
		sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGSTD));

		child_ret = stream_dvr_subprocess_to_pipe (ppfk, dvrctl);
		log_printf (LOGT_DETAIL, "Intermediate process returned (%d).\n", child_ret);
		_exit (child_ret);	/* child gone */
	}

	/* parent (regardless fork success) */

	/* safe to unblock signals now */
	sht_signalblock_mgr (SHT_OP_UNBLOCK, (SHT_F_SIGBLOCK_SIGSTD));

	if (ppfk_ret == -1) {
		log_printf (LOGT_FATAL, "Pipedfork failed.\n");
		return 6;
	}

	/* TODO: write this in a less kludgy way */
	if ((outfile = outfile_open (filename)) == NULL) {
		log_printf (LOGT_FATAL, "Unable to output stream data.\n");
		mptools_destroy_pipedfork (ppfk);	/* destroy child */
		return 4;
	}
	if ((dstf = dstf_init ()) == NULL) {
		log_printf (LOGT_FATAL, "Unable to allocate dstf.\n");
		outfile_close (outfile);
		mptools_destroy_pipedfork (ppfk);	/* destroy child */
		return 7;
	}

	/* define mc_format_out */
	switch (media_container_out) {
	case 0: mc_format_out = MC_FORM_DVR_NATIVE;	break;
	case 1: mc_format_out = MC_FORM_MKV;		break;
	}
	if (mc_format_out == MC_FORM_MKV) {
		/* MKV */
		outbuf = sbuf_2;
		outbuf_len_p = &sbuf_len_2;
	} else {
		/* DVR native: DHAV or raw H264 */
		outbuf = sbuf;
		outbuf_len_p = &sbuf_len;
	}

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
	while ((dstf->sq_maxlen - dstf->sq_len) >= (2 * STREAM_BUFFER_MAXPIPEREAD)) {
		if (poll_pipe_read (ppfk->fd_read, 100) >= 0) {
			if ((sbuf_len = read (ppfk->fd_read, sbuf, STREAM_BUFFER_MAXPIPEREAD)) == -1) {
				log_printf (LOGT_WARNING, "Unable to get more data from pipe.\n");
				goto end_stream_process;
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

		} else {
			if (sht_fl_sigchld != 0) {
				log_printf (LOGT_FATAL, "Intermediate process has died.\n");
			} else {
				log_printf (LOGT_FATAL, "Unable to communicate with intermediate process.\n");
			}
			goto end_stream_process;
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
		/* wait for data up to 100ms */
		if (poll_pipe_read (ppfk->fd_read, 100) >= 0) {
			/* there is pipe activity */
			if ((sbuf_len = read (ppfk->fd_read, sbuf, STREAM_BUFFER_MAXPIPEREAD)) == -1) {
				log_printf (LOGT_WARNING, "Unable to get more data from pipe.\n");
				break;
			}

			DEBUG_LOG_PRINTF ("read from pipe: %d bytes\n", sbuf_len);

			/* grab frames from stream, convert (if requested),
			   and send the resulting data */
			dstf_ret = dtconv_ret = outfwrite_ret = 0;
			do {
				if (mc_format_in == MC_FORM_DHAV) {
					/* MC_FORM_DHAV */
					dstf_ret = dstf_process_dhav_stream_to_frames (dstf, sbuf, (dstf_ret > 0) ? 0 : sbuf_len, sbuf, &sbuf_len, STREAM_BUFFER_LEN);
				} else {
					/* MC_FORM_RAW_H264 */
					dstf_ret = dstf_process_raw_h264_stream_to_frames (dstf, sbuf, (dstf_ret > 0) ? 0 : sbuf_len, sbuf, &sbuf_len, STREAM_BUFFER_LEN);
				}

				if (sbuf_len > 0) {
					/* there's a frame to process */

					if (mc_format_out == MC_FORM_MKV) {
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
							dtconv_ret = dt_convert_frame_to_mkv (mc_parms, sbuf, sbuf_len, sbuf_2, &sbuf_len_2, STREAM_BUFFER_LEN, main_mkv_header_pending, MCODEC_V_MPEG4_ISO_AVC);
						} else {
							/* MC_FORM_RAW_H264 */
							dtconv_ret = dt_convert_frame_to_mkv (mc_parms, sbuf, sbuf_len, sbuf_2, &sbuf_len_2, STREAM_BUFFER_LEN, main_mkv_header_pending, MCODEC_V_MPEG4_ISO_ASP);
						}
						if (dtconv_ret == 0)
							main_mkv_header_pending = false;
					}

					/* WARNING: blocking IO here */
					if ((outfwrite_ret = outfile_write (outfile, outbuf, *outbuf_len_p)) != 0)
						break;

					if ((sht_fl_terminate_nicely != 0) || (sht_fl_sigpipe != 0) || (sht_fl_sigchld != 0))
						break;
				}
			} while (dstf_ret > 0);

			if ((sht_fl_terminate_nicely != 0)) {
				log_printf (LOGT_INFO, "Got termination request.\n");
				break;
			}
			if (sht_fl_sigchld != 0) {
				log_printf (LOGT_FATAL, "Intermediate process has died.\n");
				break;
			}

			if (dstf_ret < 0) {
				log_printf (LOGT_FATAL, "dstf_process_(dhav|raw_h264)_stream_to_frames failure: %d.\n", dstf_ret);
				break;
			}
			if (dtconv_ret < 0) {
				log_printf (LOGT_FATAL, "dt_convert_frame_to_mkv failure: %d.\n", dtconv_ret);
				break;
			}
			if (outfwrite_ret != 0) {
				log_printf (LOGT_FATAL, "Unable to write to target: %d.\n", outfwrite_ret);
				break;
			}
		} else {
			if (sht_fl_sigchld != 0) {
				log_printf (LOGT_FATAL, "Intermediate process has died.\n");
			} else {
				log_printf (LOGT_FATAL, "Unable to communicate with intermediate process.\n");
			}
			break;
		}
	}

end_stream_process:

	dstf_close (dstf);
	outfile_close (outfile);
	if (dvrctl->tsproc != TSPROC_NONE) {
		dt_tsproc_close (tsc);
	}
	mc_close (mc_parms);
	mptools_destroy_pipedfork (ppfk);
	return 0;
}


