/*
	gl_model_brush.c

	gl support routines for model loading and caching

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
static const char rcsid[] = 
	"$Id$";

// models are the only shared resource between a client and server running
// on the same machine.

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include "QF/model.h"
#include "QF/qendian.h"
#include "QF/quakefs.h"
#include "QF/sys.h"
#include "QF/texture.h"
#include "QF/tga.h"
#include "QF/va.h"
#include "QF/GL/qf_textures.h"

#include "compat.h"

int   mod_lightmap_bytes = 3;


void
Mod_ProcessTexture (miptex_t *mt, texture_t *tx)
{
	char		name[32];

	snprintf (name, sizeof (name), "fb_%s", mt->name);
	tx->gl_fb_texturenum =
		Mod_Fullbright ((byte *) (tx + 1), tx->width, tx->height, name);
	tx->gl_texturenum =
		GL_LoadTexture (mt->name, tx->width, tx->height, (byte *) (tx + 1),
						true, false, 1);
}

void
Mod_LoadExternalTextures (model_t *mod)
{
	char	   *filename;
	int			i;
	tex_t	   *targa;
	texture_t  *tx;
	QFile	   *f;

	for (i = 0; i < mod->numtextures; i++) {
		tx = mod->textures[i];
		if (!tx)
			continue;

		// replace special flag characters with underscores
		if (tx->name[0] == '*') { // FIXME: translate to # or _?
		 	filename = va ("maps/#%s.tga", tx->name + 1);
		} else {
		 	filename = va ("maps/%s.tga", tx->name);
		}
		COM_FOpenFile (filename, &f);

		if (!f) {
			if (tx->name[0] == '*') {
				filename = va ("textures/#%s.tga", tx->name + 1);
			} else {
				filename = va ("textures/%s.tga", tx->name);
			}
			COM_FOpenFile (filename, &f);
		}

		if (f) {
			targa = LoadTGA (f);
			Qclose (f);
			tx->gl_texturenum =
				GL_LoadTexture (tx->name, targa->width, targa->height,
								targa->data, true, false, 3);
		}
	}
}

void
Mod_LoadLighting (lump_t *l)
{
	byte        d;
	byte       *in, *out, *data;
	char        litfilename[1024];
	int         i;

	loadmodel->lightdata = NULL;
	if (mod_lightmap_bytes > 1) {
		// LordHavoc: check for a .lit file to load
		strcpy (litfilename, loadmodel->name);
		COM_StripExtension (litfilename, litfilename);
		strncat (litfilename, ".lit", sizeof (litfilename) -
				 strlen (litfilename));
		data = (byte *) COM_LoadHunkFile (litfilename);
		if (data) {
			if (data[0] == 'Q' && data[1] == 'L' && data[2] == 'I'
				&& data[3] == 'T') {
				i = LittleLong (((int *) data)[1]);
				if (i == 1) {
					Sys_DPrintf ("%s loaded", litfilename);
					loadmodel->lightdata = data + 8;
					return;
				} else
					Sys_Printf ("Unknown .lit file version (%d)\n", i);
			} else
				Sys_Printf ("Corrupt .lit file (old version?), ignoring\n");
		}
	}
	// LordHavoc: oh well, expand the white lighting data
	if (!l->filelen)
		return;
	loadmodel->lightdata = Hunk_AllocName (l->filelen * mod_lightmap_bytes,
										   litfilename);
	in = mod_base + l->fileofs;
	out = loadmodel->lightdata;

	if (mod_lightmap_bytes > 1)
		for (i = 0; i < l->filelen ; i++) {
			d = *in++;
			*out++ = d;
			*out++ = d;
			*out++ = d;
		}
	else
		for (i = 0; i < l->filelen ; i++)
			*out++ = *in++;
}
