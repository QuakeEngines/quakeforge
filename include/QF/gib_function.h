/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2002 #AUTHOR#

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

	$Id$
*/

#ifndef __gib_function_h
#define __gib_function_h

typedef struct gib_function_s {
	struct dstring_s *name, *program;
	qboolean exported;
} gib_function_t;

void GIB_Function_Define (const char *name, const char *program);
gib_function_t *GIB_Function_Find (const char *name);
void GIB_Function_Prepare_Args (cbuf_t *cbuf, cbuf_args_t *args);
void GIB_Function_Execute (cbuf_t *cbuf, gib_function_t *func, cbuf_args_t *args);

#endif
