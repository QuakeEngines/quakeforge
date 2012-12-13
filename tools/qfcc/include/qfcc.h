/*
	qfcc.h

	QuakeForge Code Compiler (main program)

	Copyright (C) 1996-1997 id Software, Inc.
	Copyright (C) 2001 Jeff Teunissen <deek@quakeforge.net>
	Copyright (C) 2001 Bill Currie <bill@taniwha.org>

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

#ifndef __qfcc_h
#define __qfcc_h

/** \defgroup qfcc QuakeC compiler
*/

#include <stdio.h>
#include "QF/pr_comp.h"

/** \defgroup qfcc_general General functions
	\ingroup qfcc
*/
//@{

typedef struct srcline_s srcline_t;
struct srcline_s {
	srcline_t  *next;
	string_t    source_file;
	int         source_line;
};

/**	Output generated by prog parsing.
*/
typedef struct pr_info_s {
	struct type_s	*types;

	struct function_s *func_head;
	struct function_s **func_tail;
	dfunction_t		*functions;
	int				num_functions;

	struct strpool_s *strings;			///< progs string data
	struct codespace_s *code;			///< progs code data
	struct defspace_s *near_data;		///< data directly addressable by
										///< statments (address < 64k)
	struct defspace_s *far_data;		///< data that might not be directly
										///< addressabe by statements (address
										///< possibly > 64k)
	struct defspace_s *entity_data;		///< entity field address space. no
										///< data is stored in the progs file
	struct defspace_s *type_data;		///< encoded type information.

	struct symtab_s *symtab;
	struct symtab_s *entity_fields;

	srcline_t      *srcline_stack;
	string_t        source_file;
	int             source_line;
	int             error_count;

	struct reloc_s *relocs;

	struct pr_auxfunction_s *auxfunctions;
	int             auxfunctions_size;
	int             num_auxfunctions;

	struct pr_lineno_s *linenos;
	int             linenos_size;
	int             num_linenos;
} pr_info_t;

extern	pr_info_t	pr;

#define GETSTR(s)			(pr.strings->strings + (s))
#define D_var(t, d)			((d)->space->data[(d)->offset].t##_var)
#define	D_FLOAT(d)			D_var (float, d)
#define	D_INT(d)			D_var (integer, d)
#define	D_VECTOR(d)			D_var (vector, d)
#define	D_QUAT(d)			D_var (quat, d)
#define	D_STRING(d)			D_var (string, d)
#define	D_GETSTR(d)			GETSTR (D_STRING (d))
#define	D_FUNCTION(d)		D_var (func, d)
#define D_POINTER(t,d)		((t *)((d)->space->data + (d)->offset))
#define D_STRUCT(t,d)		(*D_POINTER (t, d))

#define G_POINTER(s,t,o)	((t *)((s)->data + o))
#define G_STRUCT(s,t,o)		(*G_POINTER (s, t, o))

#define POINTER_OFS(s,p)	((pr_type_t *) (p) - (s)->data)

const char *strip_path (const char *filename);

extern FILE *qc_yyin;
extern FILE *qp_yyin;
int qc_yyparse (void);
int qp_yyparse (void);
extern int qc_yydebug;
extern int qp_yydebug;

#ifdef _WIN32
char *fix_backslash (char *path);
#define NORMALIZE(x) fix_backslash (x)
#else
#define NORMALIZE(x) x
#endif

/**	Round \a x up to the next multiple of \a a.
	\note \a a must be a power of two or this will break.
	\note There are no side effects on \a x.
	\param x		The value to be rounded up.
	\param a		The rounding factor.
	\return			The rounded value.
*/
#define RUP(x,a) (((x) + ((a) - 1)) & ~((a) - 1))

//@}

#endif//__qfcc_h
