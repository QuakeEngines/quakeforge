/*
	def.h

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

#ifndef __def_h
#define __def_h

#include "QF/pr_comp.h"
#include "QF/pr_debug.h"

typedef struct def_s {
	struct type_s	*type;
	const char		*name;
	int				ofs;

	struct reloc_s *refs;			// for relocations

	unsigned		initialized:1;	// for uninit var detection
	unsigned		constant:1;	// 1 when a declaration included "= immediate"
	unsigned		freed:1;		// already freed from the scope
	unsigned		removed:1;		// already removed from the symbol table
	unsigned		used:1;			// unused local detection
	unsigned		global:1;		// globally declared def
	unsigned		absolute:1;		// don't relocate (for temps for shorts)
	unsigned		managed:1;		// managed temp

	string_t		file;			// source file
	int				line;			// source line

	int				users;			// ref counted temps
	struct expr_s	*expr;			// temp expr using this def

	struct def_s	*def_next;		// next def in scope
	struct def_s	*next;			// general purpose linking
	struct scope_s	*scope;			// scope the var was defined in
	struct defspace_s *space;
	struct def_s	*parent;		// vector/quaternion member

	void			*return_addr;	// who allocated this
} def_t;

typedef struct defspace_s {
	struct defspace_s *next;
	struct locref_s *free_locs;
	pr_type_t  *data;
	int         size;
	int         max_size;
	int       (*grow) (struct defspace_s *space);
} defspace_t;

typedef enum {
	sc_global,
	sc_params,
	sc_local,
} scope_type;

typedef struct scope_s {
	struct scope_s *next;
	scope_type  type;
	defspace_t *space;
	def_t      *head;
	def_t     **tail;
	int         num_defs;
	struct scope_s *parent;
} scope_t;

extern	def_t	def_ret, def_parms[MAX_PARMS];
extern	def_t	def_void;
extern	def_t	def_function;

scope_t *new_scope (scope_type type, defspace_t *space, scope_t *parent);
defspace_t *new_defspace (void);

def_t *get_def (struct type_s *type, const char *name, scope_t *scope,
				int allocate);
def_t *new_def (struct type_s *type, const char *name, scope_t *scope);
int new_location (struct type_s *type, defspace_t *space);
void free_location (def_t *def);
def_t *get_tempdef (struct type_s *type, scope_t *scope);
void free_tempdefs ();
void reset_tempdefs ();
void flush_scope (scope_t *scope, int force_used);
void def_initialized (def_t *d);


#endif//__def_h
