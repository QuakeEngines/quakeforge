/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2007 #AUTHOR#

	Author: #AUTHOR#
	Date: #DATE#

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static __attribute__ ((used)) const char rcsid[] =
	"$Id: template.c 11394 2007-03-17 03:23:39Z taniwha $";

#include "QF/console.h"
#include "QF/plugin.h"
#include "QF/view.h"
#include "QF/va.h"

#include "server.h"
#include "sv_console.h"

static void
draw_cpu (view_t *view)
{
	sv_view_t  *sv_view = view->data;
	sv_sbar_t  *sb = sv_view->obj;
	double      cpu;
	const char *cpu_str;
	const char *s;
	char       *d;

	cpu = (svs.stats.latched_active + svs.stats.latched_idle);
	cpu = 100 * svs.stats.latched_active / cpu;

	cpu_str = va ("[CPU: %3d%%]", (int) cpu);
	for (s = cpu_str, d = sb->text + view->xrel; *s; s++)
		*d++ = *s;
	if (cpu > 70.0) {
		int         i;
		for (i = 6; i < 9; i++)
			sb->text[view->xrel + i] |= 0x80;
	}
}

void
SV_Sbar_Init (void)
{
	view_t     *status;
	view_t     *view;

	if (!con_module || !con_module->data->console->status_view)
		return;
	status = con_module->data->console->status_view;

	view = view_new (0, 0, 11, 1, grav_northwest);
	view->draw = draw_cpu;
	view->data = status->data;
	view_add (status, view);
}
