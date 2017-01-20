/* high-level protocol routines */

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "llprotocol.h"
#include "hlprotocol.h"
#include "log.h"
#include "devinfo.h"
#include "bintools.h"

#define REMOTE_RETCODE_OFFSET 256

/* copy string from protocol data to a proper string type.
   dst_maxlen = dst array size (with trailing \0 accounted for).
   also filters this string, eliminating spurious characters. */
void extract_string (const uint8_t *src_p, const int src_len, char *dst_p, const int dst_maxlen)
{
	int to_do = src_len;
	int i;
	char c;

	if (to_do > (dst_maxlen - 1))
		to_do = dst_maxlen - 1;

	for (i = 0; i < to_do; i++) {
		c = (char) *(src_p++);
		if ((c < ' ') && (c != '\t')) {
			c = ' ';
		}
		*(dst_p++) = c;
	}
	*dst_p = '\0';
}


/* open connection to DVR
   required before login */
int hlp_open (t_hlp_connection *hlp_connection, const char *hostname, unsigned short int port, unsigned int timeout_us)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);

	return (llp_open (llp_connection, hostname, port, timeout_us));
}

void hlp_close (t_hlp_connection *hlp_connection)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);

	llp_close (llp_connection);
}

/* the difference between hlp_get_header and llp_get_header is that this one
   may store unexpected headers for later processing.
   TODO: implement storage for later processing -- currently this function
         merely discards unexpected headers */
/* return ==0 ok, !=0 error or timeout (no valid header data)
   TODO: implement timeout */
/* FIXME: this should NOT be called hlp_blabla, it's not really coherent with hlp_* layer.
          this should be moved to another layer and called differently */
int hlp_get_header (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t expected_cmd)
{
	int i, retcode;

	i = 1;
	while (i) {
		retcode = llp_get_header (llp_connection, ll_header);
		if (retcode != 0)
			return retcode;

		if (ll_header->llp_hd_cmd != expected_cmd) {
			DEBUG_LOG_PRINTF ("hlp_get_header: got cmd 0x%hhx instead of expected 0x%hhx\n", ll_header->llp_hd_cmd, expected_cmd);
			llp_get_discard_extdata (llp_connection, ll_header);
		} else {
			i = 0;
		}
	}
	return 0;
}

/* similar as hlp_get_header(),
   but also gets extdata if buf != NULL,
   or discards extdata if buf == NULL.

   if issue_nop != 0, sends a 'NOP' request (command: LLP_HD_NOP_REQ)
   in order to avoid being stuck at fread(), in cases the previous request
   did not return a response.

   certain requests provides a reply or not, and that apparently
   depends on the DVR model/firmware version.
   dahua OEM client software seems to use this same workaround. */
/* returns ==0 ok (got expected reply), >0 error, <0 got NOP instead (no reply)
   and updates extdata_len, if (extdata_len != NULL) && (buf != NULL) */
/* WARNING: some(most?) DVR models/firmware ONLY reply 'NOP' in
            the control connection, and do NOT reply in a
            media connection (tied to control channel by hlp_connection_relationship()) */
int hlp_get_header_and_extdata (t_llp_connection *llp_connection, t_ll_header *ll_header, uint8_t expected_cmd, int issue_nop, uint8_t *buf, uint32_t buflen, int *extdata_len)
{
	int retcode;
	int extdata_retlen;
	t_ll_header ll_header_nop;

	if (issue_nop != 0) {
		llp_send_nop (llp_connection);
	}

	retcode = llp_get_header (llp_connection, ll_header) != 0;
	if (retcode != 0) {
		DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: llp_get_header(1) returned: %d\n", retcode);
		return 1;
	}

	if (ll_header->llp_hd_cmd != expected_cmd) {
		/* unexpected reply code */

		retcode = llp_get_discard_extdata (llp_connection, ll_header);
		if (retcode != 0) {
			DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: llp_get_discard_extdata(1) returned: %d\n", retcode);
			return 2;
		}
		if ((issue_nop != 0) && (ll_header->llp_hd_cmd == LLP_HD_NOP_REPLY)) {
			/* got NOP, issued request did not reply */
			DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: got NOP (no reply), expected: 0x%hhx\n", expected_cmd);
			return -1;
		}
		/* got something else (or NOP when issue_nop == 0). this should not happen */
		DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: UNEXPECTED SITUATION -- got 0x%hhx, expected: 0x%hhx\n", ll_header->llp_hd_cmd, expected_cmd);
		return 1;
	} else {
		/* expected reply code */

		/* collect/discard extdata */
		if (buf == NULL) {
			llp_get_discard_extdata (llp_connection, ll_header);
		} else {
			extdata_retlen = llp_get_extdata_sbuff (llp_connection, ll_header, buf, buflen);
			if (extdata_retlen < 0) {
				DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: llp_get_extdata_sbuff returned: %d\n", extdata_retlen);
				return 3;
			} else if (extdata_len != NULL) {
				*extdata_len = extdata_retlen;
			}
		}

		/* discard following 'NOP' reply (expected, if issue_nop != 0) */
		if (issue_nop != 0) {
			retcode = llp_get_header (llp_connection, &ll_header_nop);
			if (retcode != 0) {
				DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: llp_get_header(2) returned: %d\n", retcode);
				return 4;
			}
			retcode = llp_get_discard_extdata (llp_connection, &ll_header_nop);
			if (retcode != 0) {
				DEBUG_LOG_PRINTF ("hlp_get_header_and_extdata: llp_get_discard_extdata(2) returned: %d\n", retcode);
				return 5;
			}
		}

		/* all fine */
	}

	return 0;
}

/* login to DVR
   required before sending commands */
/* if devinfo!=NULL, updates devinfo too */
int hlp_login (t_hlp_connection *hlp_connection, t_devinfo *devinfo, const char *user, const char *passwd)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	char user_passwd[MAX_USER_PASSWD_LEN + 3];
	int user_len, passwd_len;

	user_len = strlen (user);
	passwd_len = strlen (passwd);

	if ((user_len + passwd_len) > MAX_USER_PASSWD_LEN)
		return 1; /* user/passwd too long */

	/* send login */
	llp_init_header (&ll_header_out);

	/* try to fit user/passwd in the header, just in case
	   we're dealing with a very old DVR protocol version */
	if ((user_len <= 8) && (passwd_len <= 8)) {
		strncpy (&(ll_header_out.raw[8]), user, 8);	/* no need to be NULL-terminated */
		strncpy (&(ll_header_out.raw[16]), passwd, 8);	/* no need to be NULL-terminated */
		ll_header_out.extlen = 0;	/* no need to use extdata to transport login/passwd */
	} else {
		snprintf (user_passwd, MAX_USER_PASSWD_LEN + 3, "%s&&%s", user, passwd); /* FIXME: we're assuming sizeof(char)=sizeof(uint8_t) */
		ll_header_out.extlen = strlen (&user_passwd[0]); /* string (no trailing '\0') */
	}
	user_passwd[MAX_USER_PASSWD_LEN + 2] = '\0'; /* just in case */

	ll_header_out.llp_hd_cmd = 0xa0;
	ll_header_out.llp_hd_size_version = LLP_HD_VER_IHL;	/* protocol version */
	ll_header_out.raw[24] = 0x04;	/* ??? but the DVR expects that */
	ll_header_out.raw[25] = 0x01;	/* ??? but the DVR expects that */
	ll_header_out.raw[30] = 0xa1;	/* ??? but the DVR expects that */
	ll_header_out.raw[31] = 0xaa;	/* ??? but the DVR expects that */
	llp_send_header (llp_connection, &ll_header_out);
	llp_send_extdata (llp_connection, (uint8_t *) user_passwd, ll_header_out.extlen);

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xb0) != 0)
		return 2;

	/* TODO: get extdata, if any */
	llp_get_discard_extdata (llp_connection, &ll_header_in);

	/* updates devinfo */
	if (devinfo != NULL) {
		/* video type */
		switch (ll_header_in.raw[28]) {
		case 0:
			devinfo->video_sys = DI_VS_PAL;
			break;
		case 1:
			devinfo->video_sys = DI_VS_NTSC;
			break;
		default:
			devinfo->video_sys = DI_VS_UNKNOWN;
			break;
		}

		/* video encoder */
		switch (ll_header_in.raw[11]) {
		case 8:
			devinfo->video_enc = DI_VE_MPEG4;
			break;
		case 9:
			devinfo->video_enc = DI_VE_H264;
			break;
		default:
			devinfo->video_enc = DI_VE_UNKNOWN;
			break;
		}

		/* multiple windows preview support */
		switch (ll_header_in.raw[1]) {
		case 0:
			devinfo->mw_preview = DI_MWP_NO;
			break;
		case 1:
			devinfo->mw_preview = DI_MWP_YES;
			break;
		default:
			devinfo->mw_preview = DI_MWP_UNKNOWN;
			break;
		}

		devinfo->n_channels = ll_header_in.raw[10];
		devinfo->dev_type = ll_header_in.raw[12];
		devinfo->dev_subtype = ll_header_in.raw[13];

	}

	if (ll_header_in.raw[8] != 0)
		return ((int) ll_header_in.raw[9] + REMOTE_RETCODE_OFFSET);

	hlp_connection->unique_login_symbol = BT_LM2NV_U32((ll_header_in.raw) + 16);

	return 0;
}

int hlp_logout (t_hlp_connection *hlp_connection)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;

	/* send logout */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0x0a;

	BT_NV2LM_U32((ll_header_out.raw) + 8, (hlp_connection->unique_login_symbol));
	ll_header_out.extlen = 0;
	llp_send_header (llp_connection, &ll_header_out);

	/* some DVRs do not reply 0x0b to 0x0a requests, that is expected */
	hlp_get_header_and_extdata (llp_connection, &ll_header_in, 0x0b, 1, NULL, 0, NULL);

	return 0;
}

/* channel: between 0-15, 0-7 or 0-3 (depends on DVR model) */
/* sub_channel: 0-main, 1-extra_stream_1, 2-extra_stream_2 (may not work), 4-snapshot (may not work ???) */
int hlp_media_data_request (t_hlp_connection *hlp_connection_ctrl, t_hlp_connection *hlp_connection_data, int channel, int sub_channel, uint8_t *dest_data_p, size_t max_len, size_t *dest_data_len)
{
	t_llp_connection *llp_connection_ctrl = &(hlp_connection_ctrl->llp_connection);
	t_llp_connection *llp_connection_data = &(hlp_connection_data->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	uint8_t extdata_out[64];
	int i;

	DEBUG_LOG_PRINTF ("sending media request...\n");

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0x11;
//	BT_NV2LM_U32((ll_header_out.raw) + 8, (hlp_connection->unique_login_symbol));

	/* clean up extdata */
	for (i = 0; i < 64; i++)
		extdata_out[i] = 0;

	if (channel < 16) {
		/* OLD protocol (up to 16 channels 0-15) */

		ll_header_out.extlen = 16;
		ll_header_out.raw[8 + channel] = 1; /* monitor this channel - DVR requires either 0(no) or 1(yes) */
		ll_header_out.raw[24] = 0;	/* was 0xff does not modify previous value */

		if (llp_send_header (llp_connection_ctrl, &ll_header_out) != 0)
			return 4;

		/* define sub channel */
		extdata_out[channel] = sub_channel;

		if (llp_send_extdata (llp_connection_ctrl, (uint8_t *) extdata_out, ll_header_out.extlen) != 0)
			return 3;
	} else {
		/* EXTENDED protocol (required for >16 channels) */

		ll_header_out.raw[24] = 0;	/* was 0xff does not modify previous value */
		ll_header_out.raw[26] = 8;

		snprintf ((char *) extdata_out, 63,
			"ChannelName:%d\r\n"
			"Stream:%d\r\n"
			"Operate:1\r\n"
			"Protocol:tcp\r\n"
			"\r\n",
			channel, sub_channel);
		ll_header_out.extlen = strlen ((char *) extdata_out);

		if (llp_send_header (llp_connection_ctrl, &ll_header_out) != 0)
			return 4;

		if (llp_send_extdata (llp_connection_ctrl, (uint8_t *) extdata_out, ll_header_out.extlen) != 0)
			return 3;
	}

	DEBUG_LOG_PRINTF ("getting media reply...\n");

	/* get reply */
	if (hlp_get_header (llp_connection_data, &ll_header_in, 0xbc) != 0)
		return 2;

	/* get extdata */
	if (dest_data_p == NULL) {
		if (llp_get_discard_extdata (llp_connection_data, &ll_header_in) != 0)
			return 3;
	} else {
		/* collect a single frame
		   (other frames will come automatically but we don't get them now) */
		if (ll_header_in.extlen > max_len) {
			if (llp_get_discard_extdata (llp_connection_data, &ll_header_in) != 0)
				return 5;
			return 10;
		} else {
			DEBUG_LOG_PRINTF ("hlp_media_data_request: collecting 1st frame extdata, %d bytes\n", ll_header_in.extlen);
			*dest_data_len = ll_header_in.extlen;
			if (llp_get_extdata_sbuff (llp_connection_data, &ll_header_in, dest_data_p, *dest_data_len) < 0)
				return 4;
		}
	}

	if (ll_header_in.raw[16] != 0)
		return ((int) ll_header_in.raw[16] + REMOTE_RETCODE_OFFSET);

	return 0;
}


/* channel: between 0-15 */
int hlp_collect_media_data (t_hlp_connection *hlp_connection_data, int channel, uint8_t *dest_data_p, size_t max_len, size_t *dest_data_len)
{
	t_llp_connection *llp_connection_data = &(hlp_connection_data->llp_connection);
	t_ll_header ll_header_in;

	/* get data only, do not issue header */

	/* get reply */
	if (hlp_get_header (llp_connection_data, &ll_header_in, 0xbc) != 0)
		return 2;

	/* get extdata */
	if (dest_data_p == NULL) {
		if (llp_get_discard_extdata (llp_connection_data, &ll_header_in) != 0)
			return 12;
	} else {
		/* collect a single frame
		   (other frames will come automatically but we don't get them now) */
		if (ll_header_in.extlen > max_len) {
			if (llp_get_discard_extdata (llp_connection_data, &ll_header_in) != 0)
				return 13;
			return 10;
		} else {
			*dest_data_len = ll_header_in.extlen;
			if (llp_get_extdata_sbuff (llp_connection_data, &ll_header_in, dest_data_p, *dest_data_len) < 0)
				return 11;
		}
	}

	if (ll_header_in.raw[16] != 0)
		return ((int) ll_header_in.raw[16] + REMOTE_RETCODE_OFFSET);

	return 0;
}




/* requires:
   - hlp_open to both hlp_connection_orig and hlp_connection_new
   - previous call of hlp_login(hlp_connection_orig,...) */
/* reqchnumber: between 0-15 */
int hlp_connection_relationship (t_hlp_connection *hlp_connection_orig, t_hlp_connection *hlp_connection_new, uint8_t reqtype, uint8_t reqchnumber)
{
	t_llp_connection *llp_connection = &(hlp_connection_new->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	uint8_t extdata_out[16];
	int i;
	int retcode;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xf1;

	BT_NV2LM_U32((ll_header_out.raw) + 8, (hlp_connection_orig->unique_login_symbol));
	DEBUG_LOG_PRINTF ("hlp_connection_relationship: unique_login_symbol: %x\n", hlp_connection_orig->unique_login_symbol);
	ll_header_out.raw[12] = reqtype;	/* request type */
	ll_header_out.raw[13] = reqchnumber + 1;	/* channel number (1-255) */
	ll_header_out.raw[17] = 0;	/* 0-stablish 1-close */
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	return 0;

	/* we do not attempt to collect reply from this specific request.
	   most DVRs do reply, but some (or all starting from firmware version 3.x) do not.
	   the trick of issuing NOP request after this one does not work in this case,
	   since the NOPs remain unreplied as well (no idea why).
	   if the reply comes, it will be discarded later as a spurious one. */
}

/* send extension string (see command: 0xf4) */
/* note: EOL in extstr is "/r/n" */
int hlp_send_extension_string (t_hlp_connection *hlp_connection, const char *extstr)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int i;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xf4;
	ll_header_out.extlen = strlen (extstr);
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* FIXME: we assume sizeof(char)=sizeof(uint8_t), what is true for POSIX and most OSes */
	llp_send_extdata (llp_connection, (uint8_t *) extstr, ll_header_out.extlen);

	/* FIXME: we don't know what to do with the reply, so we just collect and discard */

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xf4) != 0)
		return 2;

	/* TODO: get extdata, if any */
	if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
		return 3;

	return 0;
}

/* DUMMY - yet to be implemented properly */
int hlp_get_work_alarm_status (t_hlp_connection *hlp_connection)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int i;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xa1;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xb1) != 0)
		return 2;

	/* TODO: get extdata, if any */
	if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
		return 3;

	return 0;
}

int hlp_keep_alive (t_hlp_connection *hlp_connection)
{
	/* apparently the DVR uses this as keep-alive
	   (which is required to be sent from time to time,
	   even is there's data/alarms being received, otherwise
	   the DVR simply closes the connection)
	   the DVR software sends this each ~13 seconds */
	return (hlp_get_work_alarm_status (hlp_connection));
}

#ifdef DEBUG
/* if nonzero!=0 dumps only entries with data != 0 */
void hlp_debug_dump_header (t_ll_header *ll_header, int nonzero)
{
	int i;
	int curdata;
	char curdata_char;

	fprintf (stderr, "HEAD DUMP info -- extlen: %d\n", ll_header->extlen);

	fprintf (stderr, "HEAD DUMP FOLLOWS:\n");
	for (i = 0; i < LLP_HEADER_SIZE; i++) {
		curdata = ll_header->raw[i];
		if ((curdata > 31) && (curdata < 128)) {
			curdata_char = curdata;
		} else {
			curdata_char = ' ';
		}
		if ((nonzero == 0) || ((nonzero != 0) && (curdata != 0))) {
			fprintf (stderr, "HEAD DUMP -- [%d] %x (%d) '%c'\n", i, curdata, curdata, curdata_char);
		}
	}
}

void hlp_debug_dump_extdata (uint8_t *extdata_p, int extdata_len)
{
	int i;
	int curdata;
	char curdata_char;

	DEBUG_LOG_PRINTF ("EXTDATA DUMP FOLLOWS (string data between braces):\n");
	fprintf (stderr, "[DEBUG] EXTDATA DUMP ["); // FIXME should use debug_log instead
	for (i = 0; i < extdata_len; i++) {
		curdata = *(extdata_p + i);
		if ((curdata > 31) && (curdata < 128)) {
			curdata_char = curdata;
		} else {
			curdata_char = ' ';
		}		
		fprintf (stderr, "%c", curdata_char); // FIXME should use debug_log instead
	}
	fprintf (stderr, "]\n"); // FIXME should use debug_log instead
}
#endif

/* updates devinfo (if not NULL) with information collected by hlp_get_system_information() */
void update_devinfo_from_system_information (t_devinfo *devinfo, const uint8_t *extdata, const int extdata_len, const t_ll_header *ll_header_in)
{
	if (devinfo == NULL) {
		return;
	}

	switch (ll_header_in->raw[8]) {
	case 2:
		/* HD */
		switch (ll_header_in->raw[13]) {
		case 0:
			devinfo->hd_type = DI_HD_ATA;
			break;
		case 1:
			devinfo->hd_type = DI_HD_SATA;
			break;
		default:
			devinfo->hd_type = DI_HD_UNKNOWN;
			break;
		}
		break;
	case 7:
		/* device serial number */
		extract_string (&extdata[0], extdata_len, devinfo->dev_sn, DI_STR_MAXLEN);
		break;
	case 8:
		/* device version */
		extract_string (&extdata[0], extdata_len, devinfo->dev_ver, DI_STR_MAXLEN);
		break;
	case 11:
		/* device type / chipset */
		extract_string (&extdata[0], extdata_len, devinfo->dev_type_chipset, DI_STR_MAXLEN);
	case 19:
		/* split screen capability */
		extract_string (&extdata[0], extdata_len, devinfo->split_screen, DI_STR_MAXLEN);
	case 20:
		/* language support */
		extract_string (&extdata[0], extdata_len, devinfo->lang_sup, DI_STR_MAXLEN);
		break;
	case 26:
		/* function list */
		extract_string (&extdata[0], extdata_len, devinfo->func_list, DI_STR_MAXLEN);
		break;
	case 27:
		/* wireless alarm capability */
		extract_string (&extdata[0], extdata_len, devinfo->wi_alarm, DI_STR_MAXLEN);
		break;
	case 29:
		/* OEM info */
		extract_string (&extdata[0], extdata_len, devinfo->oem_info, DI_STR_MAXLEN);
		break;
	case 30:
		/* network running status */
		extract_string (&extdata[0], extdata_len, devinfo->netrun_stat, DI_STR_MAXLEN);
		break;
	case 31:
		/* function capability */
		extract_string (&extdata[0], extdata_len, devinfo->func_capability, DI_STR_MAXLEN);
		break;
	case 33:
		/* HD partition capability */
		extract_string (&extdata[0], extdata_len, devinfo->hd_part_cap, DI_STR_MAXLEN);
		break;
	case 39:
		/* access URL */
		extract_string (&extdata[0], extdata_len, devinfo->access_url, DI_STR_MAXLEN);
		break;
	}
}

/* DUMMY - yet to be implemented properly */
/* bitstream (bitstream number) is used only when searching for information on video capability */
/* if devinfo!=NULL, updates devinfo too */
#define SYSINFO_ED_LEN 1024
int hlp_get_system_information (t_hlp_connection *hlp_connection, t_devinfo *devinfo, uint8_t infotype, uint8_t bitstream)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	uint8_t extdata[SYSINFO_ED_LEN];
	int extdata_len;

	/* send request */
	DEBUG_LOG_PRINTF ("Querying system information. Infotype: %d - Bitstream: %d\n", (int) infotype, (int) bitstream);
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xa4;
	ll_header_out.raw[8] = infotype;
	ll_header_out.raw[12] = bitstream;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xb4) != 0)
		return 2;

#ifdef DEBUG
	hlp_debug_dump_header (&ll_header_in, 1);
#endif

	/* get reply's extdata */

	if ((extdata_len = llp_get_extdata_sbuff (llp_connection, &ll_header_in, &extdata[0], SYSINFO_ED_LEN)) < 0) {
		llp_get_discard_extdata (llp_connection, &ll_header_in);
		DEBUG_LOG_PRINTF ("hlp_get_system_information got %d bytes for extdata, buf buffer is %d bytes.\n", (int) ll_header_in.extlen, SYSINFO_ED_LEN);
		return 3;	/* buffer size too small for extdata */
	}

#ifdef DEBUG
	hlp_debug_dump_extdata (&extdata[0], extdata_len);
#endif

	/* updates devinfo */
	update_devinfo_from_system_information (devinfo, extdata, extdata_len, &ll_header_in);

	return 0;
}

/* similar function as hlp_get_system_information(), except it queries for several information at once.
   this is used to long delays (several seconds in some cases) due to higher latency between the client and the DVR. */
/* if devinfo!=NULL, updates devinfo too */
int hlp_get_system_information_multiple (t_hlp_connection *hlp_connection, t_devinfo *devinfo, uint8_t first_infotype, int total_infotype)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	uint8_t extdata[SYSINFO_ED_LEN];
	int extdata_len;
	int infotype;	/* actually uint8_t, defined as int for loop purposes */
	const uint8_t bitstream = 0;
	uint8_t reply_type;

	/* send information requests */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xa4;
	ll_header_out.raw[8] = infotype;
	ll_header_out.raw[12] = bitstream;
	//
	for (infotype = first_infotype; infotype < ((int) first_infotype + total_infotype); infotype++) {
		DEBUG_LOG_PRINTF ("Multiple-Querying system information. Send request for Infotype: %d\n", (int) infotype);
		ll_header_out.raw[8] = infotype;
		if (llp_send_header (llp_connection, &ll_header_out) != 0) {
			return 4;
		}
	}
	// send 'nop' in order to detect end of stream of replies
	if (llp_send_nop (llp_connection) != 0) {
		return 5;
	}

	/* collect replies */
	/* repeat until receiving reply to 'nop' */
	while (1) {
		if (llp_get_header (llp_connection, &ll_header_in) != 0) {
			return 2;
		}
		reply_type = ll_header_in.llp_hd_cmd;

#ifdef DEBUG
		if ((reply_type != 0xb4) && (reply_type != LLP_HD_NOP_REPLY)) {
			DEBUG_LOG_PRINTF ("hlp_get_system_information_multiple: unexpected reply type: %d\n", infotype);
		}
#endif

		/* get reply's extdata */
		if ((extdata_len = llp_get_extdata_sbuff (llp_connection, &ll_header_in, &extdata[0], SYSINFO_ED_LEN)) < 0) {
			llp_get_discard_extdata (llp_connection, &ll_header_in);
			DEBUG_LOG_PRINTF ("hlp_get_system_information got %d bytes for extdata, buf buffer is %d bytes.\n", (int) ll_header_in.extlen, SYSINFO_ED_LEN);
			return 3;	/* buffer size too small for extdata */
		}

		if (reply_type == 0xb4) {
			infotype = ll_header_in.raw[8];
			DEBUG_LOG_PRINTF ("hlp_get_system_information_multiple -- collected info for: %d\n", (int) infotype);
#ifdef DEBUG
			hlp_debug_dump_extdata (&extdata[0], extdata_len);
#endif
		}

		if (reply_type == LLP_HD_NOP_REPLY) {
			DEBUG_LOG_PRINTF ("hlp_get_system_information_multiple -- got all replies, done here.\n");
			break;
		}

		/* updates devinfo */
		update_devinfo_from_system_information (devinfo, extdata, extdata_len, &ll_header_in);
	}

	return 0;
}



/* DUMMY - yet to be implemented properly */
int hlp_get_media_capability (t_hlp_connection *hlp_connection, uint8_t infotype)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int i;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0x83;
	ll_header_out.raw[9] = infotype;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0x83) != 0)
		return 2;

	/* TODO: get extdata, if any */
	if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
		return 3;

	return 0;
}

/* DUMMY - yet to be implemented properly */
int hlp_get_config_parameter (t_hlp_connection *hlp_connection, uint8_t infotype)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int i;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xa3;
	ll_header_out.raw[8] = 'c';
	ll_header_out.raw[9] = 'o';
	ll_header_out.raw[10] = 'n';
	ll_header_out.raw[11] = 'f';
	ll_header_out.raw[12] = 'i';
	ll_header_out.raw[13] = 'g';
	ll_header_out.raw[16] = infotype;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xb3) != 0)
		return 2;

	/* TODO: get extdata, if any */
	if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
		return 3;

	return 0;
}

/* DVR software subscribes to alarm as follows:

command: 68

08: 02
12: 02,... (0x01, 0x02-0x10, 0x12-0x14, 0x9c, 0xa1-0xa2)
all the other fields: 0x00 (no extdata)
*/
int hlp_set_alarm (t_hlp_connection *hlp_connection, uint8_t alarm_type)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int i;

	/* send request */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0x68;
	ll_header_out.raw[8] = 2;
	ll_header_out.raw[12] = alarm_type;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	i = 1;
	while (i) {
		/* get reply */
		if (hlp_get_header (llp_connection, &ll_header_in, 0x69) != 0)
			return 2;

		if (ll_header_out.raw[8] != 2) {
			DEBUG_LOG_PRINTF ("expected response with operation type 2, but received %x\n", (int) ll_header_out.raw[8]);
			llp_get_discard_extdata (llp_connection, &ll_header_in);
		} else if (ll_header_in.raw[12] != alarm_type) {
			DEBUG_LOG_PRINTF ("expected response for alarm type %x, but received for alarm type %x\n", (int) alarm_type, (int) ll_header_in.raw[12]);
			if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
				return 5;
		} else {
			i = 0;
		}
	}

	/* TODO: get extdata, if any */
	if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0)
		return 3;

	return 0;
}

#define CHNAMES_SIZE 512
/* provides pointer string with channel names in *outb, separated each by '\n'.
   this string should be free()'d later. */
int hlp_get_channel_names (t_hlp_connection *hlp_connection, char **outb)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_out, ll_header_in;
	int prev_was_amp;
	uint8_t *src;
	char *dst;
	char *outstr = NULL;

	if ((outstr = malloc (sizeof (char) * CHNAMES_SIZE)) == NULL)
		return 10;

	/* apparently there are two types of querying and a DVR
	   will support only of those, so it will generate one
	   reply only (to the one it understands) */
	/* send request #1 (untested reply) */
	llp_init_header (&ll_header_out);
	ll_header_out.llp_hd_cmd = 0xa8;
	llp_send_header (llp_connection, &ll_header_out);
	/* send request #2 */
//	ll_header_out.raw[8] = 0;
	ll_header_out.raw[8] = 1;
	if (llp_send_header (llp_connection, &ll_header_out) != 0)
		return 4;

	/* get reply */
	if (hlp_get_header (llp_connection, &ll_header_in, 0xb8) != 0) {
		free (outstr);
		return 2;
	}

	if (llp_get_extdata_sbuff (llp_connection, &ll_header_in, outstr, CHNAMES_SIZE) < 0)
		return 3;

	/* convert string to a more palatable format */
	src = outstr;
	dst = outstr;
	prev_was_amp = 0;
	while (*src != '\0') {
		if (*src == '&') {
			if (prev_was_amp == 0) {
				*(dst++) = '\n';
				prev_was_amp = 1;
			}
		} else {
			*(dst++) = *src;
			prev_was_amp = 0;
		}

		src++;
	}
	*(dst++) = '\n';
	*dst = '\0';

	/* copy result to the final destination */
	*outb = outstr;

	return 0;
}

/* discard whatever is pending to be readen
   returns: >0 there was indeed pending data, ==0 no pending data, <0 network ERROR */
int hlp_discard_incoming_data (t_hlp_connection *hlp_connection)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);
	t_ll_header ll_header_in;
	int i = 0;

	while (llp_check_incoming_data (llp_connection)) {
		if (llp_get_header (llp_connection, &ll_header_in) != 0) {
			i = -1;
			break;
		}
		if (llp_get_discard_extdata (llp_connection, &ll_header_in) != 0) {
			i = -1;
			break;
		}
		DEBUG_LOG_PRINTF ("Discarded incoming data.\n");
		i = 1;
	}

	return i;
}

/* check whether is pending to be readen
   returns: !=0 there was indeed pending data, ==0 no pending data */
int hlp_check_incoming_data (t_hlp_connection *hlp_connection)
{
	t_llp_connection *llp_connection = &(hlp_connection->llp_connection);

	return llp_check_incoming_data (llp_connection);
}

/* wait for one or more connections to have incoming data, or until wait_timeout (ms) is reached.
   returns: >0 incoming data, ==0 no data, <0 error */
int hlp_wait_for_incoming_data (t_hlp_connection **hlp_connection, int n_hlp_connections, int wait_timeout)
{
	t_llp_connection *llp_connection[n_hlp_connections];
	int i;

	for (i = 0; i < n_hlp_connections; i++) {
		llp_connection[i] = &(hlp_connection[i]->llp_connection);
	}
	return (llp_wait_for_incoming_data (&llp_connection[0], n_hlp_connections, wait_timeout));
}

