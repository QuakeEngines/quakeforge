/*
	qfcc.h

	#DESCRIPTION#

	Copyright (C) 2001 #AUTHOR#

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

#ifndef __qfcc_h
#define __qfcc_h

#include <stdio.h>
#include "QF/pr_comp.h"

//============================================================================

#define	MAX_REGS		65536

//=============================================================================

//
// output generated by prog parsing
//
typedef struct pr_info_s {
	struct type_s	*types;
	struct ex_label_s *labels;
	
	struct strpool_s *strings;

	dstatement_t	*statements;
	int				num_statements;
	int				statements_size;

	struct function_s *func_head;
	struct function_s **func_tail;
	dfunction_t		*functions;
	int				num_functions;

	struct defspace_s *near_data;
	struct defspace_s *far_data;
	struct defspace_s *entity_data;
	struct scope_s *scope;

	string_t        source_file;
	int             source_line;
	int             error_count;
} pr_info_t;

extern	pr_info_t	pr;

//============================================================================

extern	char		destfile[];

extern	struct scope_s *current_scope;

#define G_GETSTR(s)		(pr.strings->strings + (s))
#define G_var(t, o)		(pr.near_data->data[o].t##_var)
#define	G_FLOAT(o)		G_var (float, o)
#define	G_INT(o)		G_var (integer, o)
#define	G_VECTOR(o)		G_var (vector, o)
#define	G_STRING(o)		G_GETSTR (G_var (string, o))
#define	G_FUNCTION(o)	G_var (func, o)
#define G_POINTER(t,o)	((t *)(pr.near_data->data + o))
#define G_STRUCT(t,o)	(*G_POINTER (t, o))

#define POINTER_OFS(p)	((pr_type_t *) (p) - pr.near_data->data)

const char *strip_path (const char *filename);

const char *save_string (const char *str);
void clear_frame_macros (void);
extern FILE *yyin;
int yyparse (void);

#define ALLOC(s, t, n, v)							\
	do {											\
		if (!free_##n) {							\
			int         i;							\
			free_##n = malloc ((s) * sizeof (t));	\
			for (i = 0; i < (s) - 1; i++)			\
				free_##n[i].next = &free_##n[i + 1];\
			free_##n[i].next = 0;					\
		}											\
		v = free_##n;								\
		free_##n = free_##n->next;					\
		memset (v, 0, sizeof (*v));					\
	} while (0)

#endif//__qfcc_h
