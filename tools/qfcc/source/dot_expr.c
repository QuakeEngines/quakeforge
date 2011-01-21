/*
	dot_expr.c

	"emit" expressions to dot (graphvis).

	Copyright (C) 2011 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2011/01/20

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

static __attribute__ ((used)) const char rcsid[] = "$Id$";

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>

#include <QF/dstring.h>
#include <QF/va.h>

#include "expr.h"
#include "symtab.h"
#include "type.h"
#include "qc-parse.h"

const char *
get_op_string (int op)
{
	switch (op) {
		case PAS:	return ".=";
		case OR:	return "||";
		case AND:	return "&&";
		case EQ:	return "==";
		case NE:	return "!=";
		case LE:	return "<=";
		case GE:	return ">=";
		case LT:	return "<";
		case GT:	return ">";
		case '=':	return "=";
		case '+':	return "+";
		case '-':	return "-";
		case '*':	return "*";
		case '/':	return "/";
		case '%':	return "%";
		case '&':	return "&";
		case '|':	return "|";
		case '^':	return "^";
		case '~':	return "~";
		case '!':	return "!";
		case SHL:	return "<<";
		case SHR:	return ">>";
		case '.':	return ".";
		case 'i':	return "<if>";
		case 'n':	return "<ifnot>";
		case IFBE:	return "<ifbe>";
		case IFB:	return "<ifb>";
		case IFAE:	return "<ifae>";
		case IFA:	return "<ifa>";
		case 'g':	return "<goto>";
		case 'r':	return "<return>";
		case 's':	return "<state>";
		case 'c':	return "<call>";
		case 'C':	return "<cast>";
		case 'M':	return "<move>";
		default:
			return "unknown";
	}
}

static void
_print_expr (expr_t *e, int level, int id)
{
	const char *label = "???";
	const char *shape = "ellipse";
	int         indent = level * 2 + 2;
	if (!e) {
		printf ("%*se_%p [label=\"(null)\"];\n", indent, "", e);
		return;
	}
	if (e->printid == id)		// already printed this expression
		return;
	e->printid = id;
	switch (e->type) {
		case ex_error:
			label = "(error)";
			break;
		case ex_state:
			_print_expr (e->e.state.frame, level, id);
			_print_expr (e->e.state.think, level, id);
			if (e->e.state.step)
				_print_expr (e->e.state.step, level, id);
			printf ("%*se_%p:f -> e_%p;\n", indent, "", e, e->e.state.frame);
			printf ("%*se_%p:t -> e_%p;\n", indent, "", e, e->e.state.think);
			if (e->e.state.step)
				printf ("%*se_%p:s -> e_%p;\n", indent, "",
						e, e->e.state.step);
			shape = "record";
			label = va ("<f>state|<t>think|<s>step;");
			break;
		case ex_bool:
			_print_expr (e->e.bool.e, level, id);
			if (e->e.bool.e->type == ex_block && e->e.bool.e->e.block.head) {
				expr_t     *se;

				printf ("%*se_%p -> e_%p;\n", indent, "", e, e->e.bool.e);
				se = (expr_t *) e->e.bool.e->e.block.tail;
				if (se && se->type == ex_label && e->next)
					printf ("%*se_%p -> e_%p "
							"[constraint=false,style=dashed];\n", indent, "",
							se, e->next);
			} else {
				printf ("%*se_%p -> e_%p;\n", indent, "", e, e->e.bool.e);
			}
			label = "<bool>";
			break;
		case ex_label:
			if (e->next)
				printf ("%*se_%p -> e_%p "
						"[constraint=false,style=dashed];\n", indent, "",
						e, e->next);
			label = e->e.label.name;
			break;
		case ex_block:
			{
				expr_t     *se;

				label = "<block>";
				if (e->e.block.result) {
					_print_expr (e->e.block.result, level + 1, id);
					printf ("%*se_%p -> e_%p;\n", indent, "",
							e, e->e.block.result);
				}
				printf ("%*se_%p -> e_%p [style=dashed];\n", indent, "",
						e, e->e.block.head);
				printf ("%*ssubgraph cluster_%p {\n", indent, "", e);
				for (se = e->e.block.head; se; se = se->next) {
					_print_expr (se, level + 1, id);
				}
				for (se = e->e.block.head; se && se->next; se = se->next) {
					if ((se->type == ex_uexpr && se->e.expr.op == 'g')
						|| se->type == ex_label || se->type == ex_bool)
						continue;
					printf ("%*se_%p -> e_%p "
							"[constraint=false,style=dashed];\n", indent, "",
							se, se->next);
				}
				if (se && se->type == ex_label && e->next)
					printf ("%*se_%p -> e_%p "
							"[constraint=false,style=dashed];\n", indent, "",
							se, e->next);
				printf ("%*s}\n", indent, "");
			}
			break;
		case ex_expr:
			if (e->e.expr.op == 'c') {
				expr_t     *p;
				int         i;
				_print_expr (e->e.expr.e1, level, id);
				printf ("%*sp_%p [label=\"", indent, "", e);
				for (p = e->e.expr.e2, i = 0; p; p = p->next, i++)
					printf ("<p%d>p%d%s", i, i, p->next ? "|" : "");
				printf ("\",shape=record];\n");
				for (p = e->e.expr.e2, i = 0; p; p = p->next, i++) {
					_print_expr (p, level + 1, id);
					printf ("%*sp_%p:p%d -> e_%p;\n", indent + 2, "", e, i, p);
				}
				printf ("%*se_%p -> e_%p;\n", indent, "", e, e->e.expr.e1);
				printf ("%*se_%p -> p_%p;\n", indent, "", e, e);
			} else if (e->e.expr.op == 'i' || e->e.expr.op == 'n'
					   || e->e.expr.op == IFB || e->e.expr.op ==IFBE
					   || e->e.expr.op == IFA || e->e.expr.op ==IFAE) {
				_print_expr (e->e.expr.e1, level, id);
				printf ("%*se_%p -> e_%p [label=\"t\"];\n", indent, "",
						e, e->e.expr.e1);
				printf ("%*se_%p -> e_%p [label=\"g\"];\n", indent, "",
						e, e->e.expr.e2);
			} else {
				_print_expr (e->e.expr.e1, level, id);
				_print_expr (e->e.expr.e2, level, id);
				printf ("%*se_%p -> e_%p [label=\"l\"];\n", indent, "",
						e, e->e.expr.e1);
				printf ("%*se_%p -> e_%p [label=\"r\"];\n", indent, "",
						e, e->e.expr.e2);
			}
			label = get_op_string (e->e.expr.op);
			break;
		case ex_uexpr:
			if (e->e.expr.op != 'g')
				_print_expr (e->e.expr.e1, level, id);
			printf ("%*se_%p -> e_%p;\n", indent, "", e, e->e.expr.e1);
			label = get_op_string (e->e.expr.op);
			break;
		case ex_symbol:
			label = e->e.symbol->name;
			break;
		case ex_temp:
			label = va ("tmp_%p", e);
			break;
		case ex_nil:
			label = "nil";
			break;
		case ex_value:
			switch (e->e.value.type) {
				case ev_string:
					label = va ("\\\"%s\\\"", e->e.value.v.string_val);
					break;
				case ev_float:
					label = va ("%g", e->e.value.v.float_val);
					break;
				case ev_vector:
					label = va ("'%g %g %g'",
								e->e.value.v.vector_val[0],
								e->e.value.v.vector_val[1],
								e->e.value.v.vector_val[2]);
					break;
				case ev_quat:
					label = va ("'%g %g %g %g'",
								e->e.value.v.quaternion_val[0],
								e->e.value.v.quaternion_val[1],
								e->e.value.v.quaternion_val[2],
								e->e.value.v.quaternion_val[3]);
					break;
				case ev_pointer:
					label = va ("(%s)[%d]",
							pr_type_name[e->e.value.v.pointer.type->type],
							e->e.value.v.pointer.val);
					break;
				case ev_field:
					label = va ("field %d", e->e.value.v.pointer.val);
					break;
				case ev_entity:
					label = va ("ent %d", e->e.value.v.integer_val);
					break;
				case ev_func:
					label = va ("func %d", e->e.value.v.integer_val);
					break;
				case ev_integer:
					label = va ("%d", e->e.value.v.integer_val);
					break;
				case ev_short:
					label = va ("%d", e->e.value.v.short_val);
					break;
				case ev_void:
				case ev_invalid:
				case ev_type_count:
					internal_error (e, "weird expression type");
			}
	}
	if (label)
		printf ("%*se_%p [label=\"%s\",shape=%s];\n", indent, "",
				e, label, shape);
}

void
print_expr (expr_t *e)
{
	static int id = 0;
	printf ("digraph expr_%p {\n", e);
	printf ("  layout=dot; rankdir=TB; compound=true;\n");
	_print_expr (e, 0, ++id);
	printf ("}\n");
}
