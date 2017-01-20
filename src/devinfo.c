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

#include "devinfo.h"

void di_init (t_devinfo *devinfo) {
	devinfo->video_sys = DI_VS_UNDEFINED;
	devinfo->video_enc = DI_VE_UNDEFINED;
	devinfo->mw_preview = DI_MWP_UNDEFINED;
	devinfo->n_channels = -1;
	devinfo->dev_type = -1;
	devinfo->dev_subtype = -1;

	devinfo->hd_type = DI_HD_UNDEFINED;
	devinfo->dev_sn[0] = '\0';
	devinfo->dev_ver[0] = '\0';

	devinfo->lang_sup[0] = '\0';
	devinfo->func_list[0] = '\0';
	devinfo->oem_info[0] = '\0';
	devinfo->netrun_stat[0] = '\0';
	devinfo->split_screen[0] = '\0';
	devinfo->func_capability[0] = '\0';
	devinfo->hd_part_cap[0] = '\0';
	devinfo->wi_alarm[0] = '\0';
	devinfo->access_url[0] = '\0';
}

