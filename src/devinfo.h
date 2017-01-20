/* device information routines */

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

#ifndef HAS_DEVINFO_H
#define HAS_DEVINFO_H

#define DI_STR_MAXLEN 1024

typedef enum {DI_VS_UNDEFINED, DI_VS_NTSC, DI_VS_PAL, DI_VS_UNKNOWN} t_di_video_sys;
typedef enum {DI_VE_UNDEFINED, DI_VE_MPEG4, DI_VE_H264, DI_VE_UNKNOWN} t_di_video_enc;
typedef enum {DI_MWP_UNDEFINED, DI_MWP_YES, DI_MWP_NO, DI_MWP_UNKNOWN} t_di_mw_preview;
typedef enum {DI_HD_UNDEFINED, DI_HD_ATA, DI_HD_SATA, DI_HD_UNKNOWN} t_di_hdtype;

typedef struct {
	t_di_video_sys video_sys;
	t_di_video_enc video_enc;
	t_di_mw_preview mw_preview;
	int n_channels;	/* <0 if undefined */
	int dev_type; /* ??? -- <0 if undefined */
	int dev_subtype; /* ??? "derivative device type" -- <0 if undefined */

	t_di_hdtype hd_type;
	char dev_sn[DI_STR_MAXLEN];
	char dev_ver[DI_STR_MAXLEN];

	/* TODO: parse the following strings accordingly */
	char dev_type_chipset[DI_STR_MAXLEN];
	char lang_sup[DI_STR_MAXLEN];
	char func_list[DI_STR_MAXLEN];
	char oem_info[DI_STR_MAXLEN];
	char netrun_stat[DI_STR_MAXLEN];
	char split_screen[DI_STR_MAXLEN];
	char func_capability[DI_STR_MAXLEN];
	char hd_part_cap[DI_STR_MAXLEN];
	char wi_alarm[DI_STR_MAXLEN];

	char access_url[DI_STR_MAXLEN];
} t_devinfo;


extern void di_init (t_devinfo *devinfo);

#endif

