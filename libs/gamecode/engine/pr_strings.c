/*
	pr_strings.c

	progs string managment

	Copyright (C) 1996-1997  Id Software, Inc.

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

static __attribute__ ((unused)) const char rcsid[] = 
	"$Id$";

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#include "QF/dstring.h"
#include "QF/hash.h"
#include "QF/progs.h"

struct strref_s {
	strref_t   *next;
	char       *string;
	dstring_t  *dstring;
	int         count;
};

// format adjustments
#define FMT_ALTFORM		(1<<0)
#define FMT_LJUSTIFY	(1<<1)
#define FMT_ZEROPAD		(1<<2)
#define FMT_ADDSIGN		(1<<3)
#define FMT_ADDBLANK	(1<<4)
#define FMT_HEX			(1<<5)

typedef struct fmt_item_s {
	byte        type;
	unsigned    flags;
	int         minFieldWidth;
	int         precision;
	union {
		const char *string_var;
		int         integer_var;
		unsigned    uinteger_var;
		float       float_var;
	}           data;
	struct fmt_item_s *next;
} fmt_item_t;

static strref_t *free_string_refs;
static fmt_item_t *free_fmt_items;

static void *
pr_strings_alloc (void *_pr, size_t size)
{
	progs_t    *pr = (progs_t *) _pr;
	return PR_Zone_Malloc (pr, size);
}

static void
pr_strings_free (void *_pr, void *ptr)
{
	progs_t    *pr = (progs_t *) _pr;
	PR_Zone_Free (pr, ptr);
}

static void *
pr_strings_realloc (void *_pr, void *ptr, size_t size)
{
	progs_t    *pr = (progs_t *) _pr;
	return PR_Zone_Realloc (pr, ptr, size);
}

static strref_t *
new_string_ref (progs_t *pr)
{
	strref_t *sr;
	if (!free_string_refs) {
		int		i, size;

		pr->dyn_str_size++;
		size = pr->dyn_str_size * sizeof (strref_t *);
		pr->dynamic_strings = realloc (pr->dynamic_strings, size);
		if (!pr->dynamic_strings)
			PR_Error (pr, "out of memory");
		if (!(free_string_refs = calloc (1024, sizeof (strref_t))))
			PR_Error (pr, "out of memory");
		pr->dynamic_strings[pr->dyn_str_size - 1] = free_string_refs;
		for (i = 0, sr = free_string_refs; i < 1023; i++, sr++)
			sr->next = sr + 1;
		sr->next = 0;
	}
	sr = free_string_refs;
	free_string_refs = sr->next;
	sr->next = 0;
	return sr;
}

static void
free_string_ref (progs_t *pr, strref_t *sr)
{
	sr->string = 0;
	sr->dstring = 0;
	sr->next = free_string_refs;
	free_string_refs = sr;
}

static int
string_index (progs_t *pr, strref_t *sr)
{
	long        o = (long) (sr - pr->static_strings);
	unsigned int i;

	if (o >= 0 && o < pr->num_strings)
		return sr->string - pr->pr_strings;
	for (i = 0; i < pr->dyn_str_size; i++) {
		int d = sr - pr->dynamic_strings[i];
		if (d >= 0 && d < 1024)
			return ~(i * 1024 + d);
	}
	return 0;
}

static const char *
strref_get_key (void *_sr, void *notused)
{
	strref_t	*sr = (strref_t*)_sr;

	return sr->string;
}

static void
strref_free (void *_sr, void *_pr)
{
	progs_t		*pr = (progs_t*)_pr;
	strref_t	*sr = (strref_t*)_sr;

	// Since this is only ever called by Hash_FlushTable, the memory pointed
	// to by sr->string or sr->dstring has already been lost in the progs
	// load/reload and thus there's no need to free it.

	// free the string and ref only if it's not a static string
	if (sr < pr->static_strings || sr >= pr->static_strings + pr->num_strings) {
		free_string_ref (pr, sr);
	}
}

int
PR_LoadStrings (progs_t *pr)
{
	char   *end = pr->pr_strings + pr->progs->numstrings;
	char   *str = pr->pr_strings;
	int		count = 0;

	while (str < end) {
		count++;
		str += strlen (str) + 1;
	}

	if (!pr->ds_mem) {
		pr->ds_mem = malloc (sizeof (dstring_mem_t));
		pr->ds_mem->alloc = pr_strings_alloc;
		pr->ds_mem->free = pr_strings_free;
		pr->ds_mem->realloc = pr_strings_realloc;
		pr->ds_mem->data = pr;
	}
	if (pr->strref_hash) {
		Hash_FlushTable (pr->strref_hash);
	} else {
		pr->strref_hash = Hash_NewTable (1021, strref_get_key, strref_free,
										 pr);
		pr->dynamic_strings = 0;
		free_string_refs = 0;
		pr->dyn_str_size = 0;
	}

	if (pr->static_strings)
		free (pr->static_strings);
	pr->static_strings = malloc (count * sizeof (strref_t));
	count = 0;
	str = pr->pr_strings;
	while (str < end) {
		pr->static_strings[count].string = str;
		str += strlen (str) + 1;
		Hash_Add (pr->strref_hash, &pr->static_strings[count]);
		count++;
	}
	pr->num_strings = count;
	return 1;
}

static inline strref_t *
get_strref (progs_t *pr, int num)
{
	if (num < 0) {
		unsigned int row = ~num / 1024;

		num = ~num % 1024;

		if (row >= pr->dyn_str_size)
			return 0;
		return &pr->dynamic_strings[row][num];
	} else {
		return 0;
	}
}

static inline const char *
get_string (progs_t *pr, int num)
{
	if (num < 0) {
		strref_t   *ref = get_strref (pr, num);
		if (!ref)
			return 0;
		if (ref->dstring)
			return ref->dstring->str;
		return ref->string;
	} else {
		if (num >= pr->pr_stringsize)
			return 0;
		return pr->pr_strings + num;
	}
}

qboolean
PR_StringValid (progs_t *pr, int num)
{
	return get_string (pr, num) != 0;
}

const char *
PR_GetString (progs_t *pr, int num)
{
	const char *str;

	str = get_string (pr, num);
	if (str)
		return str;
	PR_RunError (pr, "Invalid string offset %d", num);
}

dstring_t *
PR_GetDString (progs_t *pr, int num)
{
	strref_t   *ref = get_strref (pr, num);
	if (ref) {
		if (ref->dstring)
			return ref->dstring;
		PR_RunError (pr, "not a dstring: %d", num);
	}
	PR_RunError (pr, "Invalid string offset: %d", num);
}

static inline char *
pr_strdup (progs_t *pr, const char *s)
{
	size_t      len = strlen (s) + 1;
	char       *new = PR_Zone_Malloc (pr, len);
	strcpy (new, s);
	return new;
}

int
PR_SetString (progs_t *pr, const char *s)
{
	strref_t   *sr = Hash_Find (pr->strref_hash, s);

	if (!sr) {
		sr = new_string_ref (pr);
		sr->string = pr_strdup(pr, s);
		sr->count = 0;
		Hash_Add (pr->strref_hash, sr);
	}
	return string_index (pr, sr);
}

int
PR_SetTempString (progs_t *pr, const char *s)
{
	strref_t   *sr;

	if (!s)
		return PR_SetString (pr, "");

	sr = new_string_ref (pr);
	sr->string = pr_strdup(pr, s);
	sr->count = 0;
	sr->next = pr->pr_xtstr;
	pr->pr_xtstr = sr;
	return string_index (pr, sr);
}

void
PR_MakeTempString (progs_t *pr, int str)
{
	strref_t   *sr = get_strref (pr, str);

	if (!sr)
		PR_RunError (pr, "invalid string %d", str);
	if (sr->dstring) {
		if (sr->dstring->str)
			sr->string = sr->dstring->str;
		PR_Zone_Free (pr, sr->dstring);
	}
	if (!sr->string)
		sr->string = pr_strdup (pr, "");
	sr->count = 0;
	sr->next = pr->pr_xtstr;
	pr->pr_xtstr = sr;
}

int
PR_NewString (progs_t *pr)
{
	strref_t   *sr = new_string_ref (pr);
	sr->dstring = _dstring_newstr (pr->ds_mem);
	return string_index (pr, sr);
}

void
PR_FreeString (progs_t *pr, int str)
{
	strref_t   *sr = get_strref (pr, str);

	if (sr) {
		if (sr->dstring)
			dstring_delete (sr->dstring);
		else
			PR_Zone_Free (pr, sr->string);
		free_string_ref (pr, sr);
		return;
	}
	PR_RunError (pr, "attempt to free invalid string %d", str);
}

void
PR_FreeTempStrings (progs_t *pr)
{
	strref_t   *sr, *t;

	for (sr = pr->pr_xtstr; sr; sr = t) {
		t = sr->next;
		PR_Zone_Free (pr, sr->string);
		free_string_ref (pr, sr);
	}
	pr->pr_xtstr = 0;
}

#define PRINT(t)													\
	switch ((doWidth << 1) | doPrecision) {							\
		case 3:														\
			dasprintf (result, tmp->str, current->minFieldWidth,	\
					   current->precision, current->data.t##_var);	\
			break;													\
		case 2:														\
			dasprintf (result, tmp->str, current->minFieldWidth,	\
					   current->precision, current->data.t##_var);	\
			break;													\
		case 1:														\
			dasprintf (result, tmp->str, current->precision,		\
					   current->data.t##_var);						\
			break;													\
		case 0:														\
			dasprintf (result, tmp->str, current->data.t##_var);	\
			break;													\
	}

/*
	This function takes as input a linked list of fmt_item_t representing
	EVERYTHING to be printed. This includes text that is not affected by
	formatting. A string without any formatting would wind up with only one
	list item.
*/
static void
I_DoPrint (dstring_t *result, fmt_item_t *formatting)
{
	fmt_item_t		*current = formatting;
	dstring_t		*tmp = dstring_new ();

	while (current) {
		qboolean	doPrecision, doWidth;

		doPrecision = -1 != current->precision;
		doWidth = 0 != current->minFieldWidth;
		
		dsprintf (tmp, "%%%s%s%s%s%s%s%s",
			(current->flags & FMT_ALTFORM) ? "#" : "",	// hash
			(current->flags & FMT_ZEROPAD) ? "0" : "",	// zero padding
			(current->flags & FMT_LJUSTIFY) ? "-" : "",	// left justify
			(current->flags & FMT_ADDBLANK) ? " " : "",	// add space for +ve
			(current->flags & FMT_ADDSIGN) ? "+" : "",	// add sign always
			doWidth ? "*" : "",
			doPrecision ? ".*" : "");

		switch (current->type) {
			case 's':
				dstring_appendstr (tmp, "s");
				PRINT (string);
				break;
			case 'i':
				dstring_appendstr (tmp, "ld");
				PRINT (integer);
				break;
			case 'u':
				if (current->flags & FMT_HEX)
					dstring_appendstr (tmp, "lx");
				else
					dstring_appendstr (tmp, "lu");
				PRINT (uinteger);
				break;
			case 'f':
				dstring_appendstr (tmp, "f");
				PRINT (float);
				break;
			case 'g':
				dstring_appendstr (tmp, "g");
				PRINT (float);
				break;
			default:
				break;
		}
		current = current->next;
	}
	dstring_delete (tmp);
}

static fmt_item_t *
new_fmt_item (void)
{
	int        i;
	fmt_item_t *fi;

	if (!free_fmt_items) {
		free_fmt_items = malloc (16 * sizeof (fmt_item_t));
		for (i = 0; i < 15; i++)
			free_fmt_items[i].next = free_fmt_items + i + 1;
		free_fmt_items[i].next = 0;
	}

	fi = free_fmt_items;
	free_fmt_items = fi->next;
	memset (fi, 0, sizeof (*fi));
	fi->precision = -1;
	return fi;
}

static void
free_fmt_item (fmt_item_t *fi)
{
	fi->next = free_fmt_items;
	free_fmt_items = fi;
}

#undef P_var
#define P_var(p,n,t) (args[n]->t##_var)
void
PR_Sprintf (progs_t *pr, dstring_t *result, const char *name,
			const char *format, int count, pr_type_t **args)
{
	const char *c, *l;
	const char *msg = "";
	fmt_item_t *fmt_items = 0;
	fmt_item_t **fi = &fmt_items;
	int         fmt_count = 0;

	if (!name)
		name = "PF_InternalSprintf";

	*fi = new_fmt_item ();
	c = l = format;
	while (*c) {
		if (*c++ == '%') {
			if (c != l + 1) {
				// have some unformatted text to print
				(*fi)->precision = c - l - 1;
				(*fi)->type = 's';
				(*fi)->data.string_var = l;

				(*fi)->next = new_fmt_item ();
				fi = &(*fi)->next;
			}
			if (*c == '%') {
				(*fi)->type = 's';
				(*fi)->data.string_var = "%";

				(*fi)->next = new_fmt_item ();
				fi = &(*fi)->next;
			} else {
				do {
					switch (*c) {
						// format options
						case '\0':
							msg = "Unexpected end of format string";
							goto error;
						case '0':
							(*fi)->flags |= FMT_ZEROPAD;
							c++;
							continue;
						case '#':
							(*fi)->flags |= FMT_ALTFORM;
							c++;
							continue;
						case ' ':
							(*fi)->flags |= FMT_ADDBLANK;
							c++;
							continue;
						case '-':
							(*fi)->flags |= FMT_LJUSTIFY;
							c++;
							continue;
						case '+':
							(*fi)->flags |= FMT_ADDSIGN;
							c++;
							continue;
						case '.':
							(*fi)->precision = 0;
							c++;
							while (isdigit (*c)) {
								(*fi)->precision *= 10;
								(*fi)->precision += *c++ - '0';
							}
							continue;
						case '1': case '2': case '3': case '4': case '5':
						case '6': case '7': case '8': case '9':
							while (isdigit (*c)) {
								(*fi)->minFieldWidth *= 10;
								(*fi)->minFieldWidth += *c++ - '0';
							}
							continue;
						// format types
						case '@':
							// object
							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'e':
							// entity
							(*fi)->type = 'i';
							(*fi)->data.integer_var =
								P_EDICTNUM (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'i':
							// integer
							(*fi)->type = *c;
							(*fi)->data.integer_var = P_INT (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'f':
							// float
						case 'g':
							// float, no trailing zeroes, trim "." if nothing
							// after
							(*fi)->type = *c;
							(*fi)->data.float_var = P_FLOAT (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'p':
							// pointer
							(*fi)->flags |= FMT_ALTFORM;
							(*fi)->type = 'x';
							(*fi)->data.uinteger_var = P_UINT (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 's':
							// string
							(*fi)->type = *c;
							(*fi)->data.string_var = P_GSTRING (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'v':
							// vector
							{
								int         i;
								int         flags = (*fi)->flags;
								int         precision = (*fi)->precision;
								unsigned    minWidth = (*fi)->minFieldWidth;

								(*fi)->flags = 0;
								(*fi)->precision = -1;
								(*fi)->minFieldWidth = 0;

								for (i = 0; i < 3; i++) {
									if (i == 0) {
										(*fi)->type = 's';
										(*fi)->data.string_var = "'";
									} else {
										(*fi)->type = 's';
										(*fi)->data.string_var = " ";
									}
									(*fi)->next = new_fmt_item ();
									fi = &(*fi)->next;

									(*fi)->flags = flags;
									(*fi)->precision = precision;
									(*fi)->minFieldWidth = minWidth;
									(*fi)->type = 'g';
									(*fi)->data.float_var =
										P_VECTOR (pr, fmt_count)[i];

									(*fi)->next = new_fmt_item ();
									fi = &(*fi)->next;
								}
							}

							(*fi)->type = 's';
							(*fi)->data.string_var = "'";

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
						case 'x':
							// integer, hex notation
							(*fi)->type = *c;
							(*fi)->data.uinteger_var = P_UINT (pr, fmt_count);

							fmt_count++;
							(*fi)->next = new_fmt_item ();
							fi = &(*fi)->next;
							break;
					}
					break;
				} while (1);
			}
			l = ++c;
		}
	}
	if (c != l) {
		// have some unformatted text to print
		(*fi)->precision = c - l;
		(*fi)->type = 's';
		(*fi)->data.string_var = l;

		(*fi)->next = new_fmt_item ();
		fi = &(*fi)->next;
	}

	if (fmt_count != count) {
		printf ("%d %d", fmt_count, count);
		if (fmt_count > count)
			msg = "Not enough arguments for format string.";
		else
			msg = "Too many arguments for format string.";
		goto error;
	}

	I_DoPrint (result, fmt_items);
	while (fmt_items) {
		fmt_item_t *t = fmt_items->next;
		free_fmt_item (fmt_items);
		fmt_items = t;
	}
	return;
error:
	PR_RunError (pr, "%s: %s", name, msg);
}
