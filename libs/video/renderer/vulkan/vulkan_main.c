/*
	vulkan_main.c

	Vulkan rendering

	Copyright (C) 2021 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2021/1/19

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

#define NH_DEFINE
#include "namehack.h"

#ifdef HAVE_STRING_H
# include "string.h"
#endif
#ifdef HAVE_STRINGS_H
# include "strings.h"
#endif

#include "QF/cmd.h"
#include "QF/cvar.h"
#include "QF/image.h"
#include "QF/render.h"
#include "QF/screen.h"
#include "QF/sys.h"

#include "QF/Vulkan/qf_vid.h"
#include "QF/Vulkan/qf_alias.h"
#include "QF/Vulkan/qf_bsp.h"
//#include "QF/Vulkan/qf_iqm.h"
#include "QF/Vulkan/qf_lightmap.h"
#include "QF/Vulkan/qf_main.h"
#include "QF/Vulkan/qf_particles.h"
//#include "QF/Vulkan/qf_textures.h"

#include "mod_internal.h"
#include "r_internal.h"
#include "vid_vulkan.h"

static void
setup_frame (vulkan_ctx_t *ctx)
{
	R_AnimateLight ();
	R_ClearEnts ();
	r_framecount++;

	VectorCopy (r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	R_SetFrustum ();

	r_viewleaf = Mod_PointInLeaf (r_origin, r_worldentity.model);
}

static void
setup_view (vulkan_ctx_t *ctx)
{
	mat4_t      mat;
	float      *view = ctx->matrices.view_3d;
	static mat4_t z_up = {
		 0, 0, -1, 0,
		-1, 0,  0, 0,
		 0, 1,  0, 0,
		 0, 0,  0, 1,
	};

	/*x = r_refdef.vrect.x;
	y = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height));
	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;
	qfeglViewport (x, y, w, h);*/

	Mat4Zero (mat);
	VectorCopy (vpn, mat + 0);
	VectorNegate (vright, mat + 4);			// we want vleft
	VectorCopy (vup, mat + 8);
	mat[15] = 1;
	Mat4Transpose (mat, mat);//AngleVectors gives the transpose of what we want
	Mat4Mult (z_up, mat, view);

	Mat4Identity (mat);
	VectorNegate (r_refdef.vieworg, mat + 12);
	Mat4Mult (view, mat, view);
}

static void
R_RenderEntities (vulkan_ctx_t *ctx)
{
	if (!r_drawentities->int_val)
		return;
#define RE_LOOP(type_name, Type) \
	do { \
		entity_t   *ent; \
		int         begun = 0; \
		for (ent = r_ent_queue; ent; ent = ent->next) { \
			if (ent->model->type != mod_##type_name) \
				continue; \
			if (!begun) { \
				Vulkan_##Type##Begin (ctx); \
				begun = 1; \
			} \
			Vulkan_Draw##Type (ent, ctx); \
		} \
		if (begun) \
			Vulkan_##Type##End (ctx); \
	} while (0)

	RE_LOOP (alias, Alias);
	//RE_LOOP (iqm, IQM);
	//RE_LOOP (sprite, Sprite);
}

static void
R_DrawViewModel (void)
{
	currententity = vr_data.view_model;
	if (vr_data.inhibit_viewmodel
		|| !r_drawviewmodel->int_val
		|| !r_drawentities->int_val
		|| !currententity->model)
		return;

	// hack the depth range to prevent view model from poking into walls
	//qfeglDepthRangef (0, 0.3);
	//glsl_R_AliasBegin ();
	//glsl_R_DrawAlias ();
	//glsl_R_AliasEnd ();
	//qfeglDepthRangef (0, 1);
}

void
Vulkan_RenderView (vulkan_ctx_t *ctx)
{
	double      t[10] = {};
	int         speeds = r_speeds->int_val;

	if (speeds)
		t[0] = Sys_DoubleTime ();
	setup_frame (ctx);
	setup_view (ctx);
	if (speeds)
		t[1] = Sys_DoubleTime ();
	R_MarkLeaves ();
	if (speeds)
		t[2] = Sys_DoubleTime ();
	R_PushDlights (vec3_origin);
	if (speeds)
		t[3] = Sys_DoubleTime ();
	Vulkan_DrawWorld (ctx);
	if (speeds)
		t[4] = Sys_DoubleTime ();
	Vulkan_DrawSky (ctx);
	if (speeds)
		t[5] = Sys_DoubleTime ();
	R_RenderEntities (ctx);
	if (speeds)
		t[6] = Sys_DoubleTime ();
	Vulkan_DrawWaterSurfaces (ctx);
	if (speeds)
		t[7] = Sys_DoubleTime ();
	Vulkan_DrawParticles (ctx);
	if (speeds)
		t[8] = Sys_DoubleTime ();
	R_DrawViewModel ();
	if (speeds)
		t[9] = Sys_DoubleTime ();
	if (speeds) {
		Sys_Printf ("frame: %g, setup: %g, mark: %g, pushdl: %g, world: %g,"
					" sky: %g, ents: %g, water: %g, part: %g, view: %g\n",
					(t[9] - t[0]) * 1000, (t[1] - t[0]) * 1000,
					(t[2] - t[1]) * 1000, (t[3] - t[2]) * 1000,
					(t[4] - t[3]) * 1000, (t[5] - t[4]) * 1000,
					(t[6] - t[5]) * 1000, (t[7] - t[6]) * 1000,
					(t[8] - t[7]) * 1000, (t[9] - t[8]) * 1000);
	}
}

void
Vulkan_NewMap (model_t *worldmodel, struct model_s **models, int num_models,
			   vulkan_ctx_t *ctx)
{
	int         i;

	for (i = 0; i < 256; i++) {
		d_lightstylevalue[i] = 264;		// normal light value
	}

	memset (&r_worldentity, 0, sizeof (r_worldentity));
	r_worldentity.model = worldmodel;

	// Force a vis update
	r_viewleaf = NULL;
	R_MarkLeaves ();

	R_FreeAllEntities ();
	Vulkan_ClearParticles (ctx);
	Vulkan_RegisterTextures (models, num_models, ctx);
	Vulkan_BuildLightmaps (models, num_models, ctx);
	Vulkan_BuildDisplayLists (models, num_models, ctx);
}

/*void
glsl_R_LineGraph (int x, int y, int *h_vals, int count)
{
}*/

/*void
glsl_R_TimeRefresh_f (void)
{
	double      start, stop, time;
	int         i;

	glsl_ctx->end_rendering ();

	start = Sys_DoubleTime ();
	for (i = 0; i < 128; i++) {
		r_refdef.viewangles[1] = i * (360.0 / 128.0);
		Vulkan_RenderView (ctx);
		glsl_ctx->end_rendering ();
	}

	stop = Sys_DoubleTime ();
	time = stop - start;
	Sys_Printf ("%g seconds (%g fps)\n", time, 128 / time);
}*/
