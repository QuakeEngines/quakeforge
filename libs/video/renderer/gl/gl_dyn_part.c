/*
	gl_dyn_part.c

	OpenGL particle system.

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

	$Id$
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <stdlib.h>

#include "QF/cmd.h"
#include "QF/console.h"
#include "QF/cvar.h"
#include "QF/qargs.h"
#include "QF/render.h"
#include "QF/sys.h"
#include "QF/vfs.h"
#include "QF/GL/defines.h"
#include "QF/GL/funcs.h"

#include "compat.h"
#include "r_cvar.h"
#include "r_dynamic.h"
#include "r_shared.h"
#include "varrays.h"

static particle_t *particles, **freeparticles;
static short r_numparticles, numparticles;

int			ramp[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };

extern int  part_tex_dot;
extern int  part_tex_spark;
extern int  part_tex_smoke[8];

extern cvar_t *cl_max_particles;


inline particle_t *
particle_new (ptype_t type, int texnum, vec3_t org, float scale, vec3_t vel,
			  float die, byte color, byte alpha)
{
	particle_t *part;

/*
	if (numparticles >= r_numparticles) {
		Con_Printf("FAILED PARTICLE ALLOC!\n");
		return NULL;
	}
*/

	part = &particles[numparticles++];

	part->type = type;
	VectorCopy (org, part->org);
	VectorCopy (vel, part->vel);
	part->die = die;
	part->color = color;
	part->alpha = alpha;
	part->tex = texnum;
	part->scale = scale;

	return part;
}

inline particle_t *
particle_new_random (ptype_t type, int texnum, vec3_t org, int org_fuzz,
					 float scale, int vel_fuzz, float die, byte color,
					 byte alpha)
{
	int         j;
	vec3_t      porg, pvel;

	for (j = 0; j < 3; j++) {
		if (org_fuzz)
			porg[j] = qfrandom (org_fuzz * 2) - org_fuzz + org[j];
		if (vel_fuzz)
			pvel[j] = qfrandom (vel_fuzz * 2) - vel_fuzz;
	}
	return particle_new (type, texnum, porg, scale, pvel, die, color, alpha);
}

/*
  R_MaxParticlesCheck

  Misty-chan: Dynamically change the maximum amount of particles on the fly.
  Thanks to a LOT of help from Taniwha, Deek, Mercury, Lordhavoc, and lots of
  others.
*/
void
R_MaxParticlesCheck (cvar_t *var)
{
/*
	Catchall. If the user changed the setting to a number less than zero *or*
	if we had a wacky cfg get past the init code check, this will make sure we
	don't have problems. Also note that grabbing the var->int_val is IMPORTANT:

	Prevents a segfault since if we grabbed the int_val of cl_max_particles
	we'd sig11 right here at startup.
*/
	r_numparticles = max(var->int_val, 0);

/*
	Be very careful the next time we do something like this. calloc/free are
	IMPORTANT and the compiler doesn't know when we do bad things with them.
*/
	free (particles);
	free (freeparticles);

	particles = (particle_t *) calloc (r_numparticles, sizeof (particle_t));
	freeparticles = (particle_t **)
		calloc (r_numparticles, sizeof (particle_t *));

	R_ClearParticles();
}

void
R_Particles_Init_Cvars (void)
{
/*
	Misty-chan: This is a cvar that does callbacks. Whenever it
	changes, it calls the function R_MaxParticlesCheck and therefore
	is very nifty.
*/
	Cvar_Get ("cl_max_particles", "2048", CVAR_ARCHIVE, R_MaxParticlesCheck,
			  "Maximum amount of particles to display. No maximum, minimum "
			  "is 0, although it's best to use r_particles 0 instead.");
}

inline void
R_ClearParticles (void)
{
	numparticles = 0;
}

void
R_ReadPointFile_f (void)
{
	char        name[MAX_OSPATH], *mapname, *t1;
	int         c, r;
	vec3_t      org;
	VFile      *f;

	mapname = strdup (r_worldentity.model->name);
	if (!mapname)
		Sys_Error ("Can't duplicate mapname!");
	t1 = strrchr (mapname, '.');
	if (!t1)
		Sys_Error ("Can't find .!");
	t1[0] = '\0';

	snprintf (name, sizeof (name), "%s.pts", mapname);
	free (mapname);

	COM_FOpenFile (name, &f);
	if (!f) {
		Con_Printf ("couldn't open %s\n", name);
		return;
	}

	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for (;;) {
		char        buf[64];

		Qgets (f, buf, sizeof (buf));
		r = sscanf (buf, "%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;

		if (!particle_new (pt_static, part_tex_dot, org, 1.5, vec3_origin,
						   99999, (-c) & 15, 255)) {
			Con_Printf ("Not enough free particles\n");
			break;
		}
	}

	Qclose (f);
	Con_Printf ("%i points read\n", c);
}

void
R_ParticleExplosion (vec3_t org)
{
//	int			i;
//	int			j = 1024;

	if (!r_particles->int_val)
		return;

	if (numparticles >= r_numparticles)
		return;
// 	else if (numparticles + j >= r_numparticles)
//		j = r_numparticles - numparticles;
		
	particle_new_random (pt_smokecloud, part_tex_smoke[rand () & 7], org, 4,
						 30, 8, r_realtime + 5, (rand () & 7) + 8,
						 128 + (rand () & 63));
/*
	for (i=0; i < j; i++) {
		particle_new_random (pt_fallfadespark, part_tex_spark, org, 16,
							 1.5, 256, r_realtime + 5, ramp[rand () & 7],
							 255);
	}
*/
}

void
R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int         i;
	int         colorMod = 0, j = 512;

	if (!r_particles->int_val)
		return;
	
	if (numparticles >= r_numparticles)
		return;
	else if (numparticles + j >= r_numparticles)
		j = r_numparticles - numparticles;

	for (i = 0; i < j; i++) {
		particle_new_random (pt_blob, part_tex_dot, org, 16, 2, 256,
							 (r_realtime + 0.3),
							 (colorStart + (colorMod % colorLength)), 255);
		colorMod++;
	}
}

void
R_BlobExplosion (vec3_t org)
{
	int         i;
	int			j = 1024;

	if (!r_particles->int_val)
		return;

	if (numparticles >= r_numparticles)
		return;
	else if (numparticles + j >= r_numparticles)
		j = r_numparticles - numparticles;

	for (i = 0; i < j / 2; i++) {
		particle_new_random (pt_blob, part_tex_dot, org, 12, 2, 256,
							 (r_realtime + 1 + (rand () & 7) * 0.05),
							 (66 + rand () % 6), 255);
	}
	for (i = 0; i < j / 2; i++) {
		particle_new_random (pt_blob2, part_tex_dot, org, 12, 2, 256,
							 (r_realtime + 1 + (rand () & 7) * 0.05),
							 (150 + rand () % 6), 255);
	}
}

static void
R_RunSparkEffect (vec3_t org, int count, int ofuzz)
{
	int			j = count + 1;

	if (numparticles >= r_numparticles)
		return;
	else if (numparticles + j >= r_numparticles)
		j = r_numparticles - numparticles;
	count = j - 1;

	particle_new (pt_smokecloud, part_tex_smoke[rand () & 7], org,
				  ofuzz * 0.08, vec3_origin, r_realtime + 9,
				  12 + (rand () & 3), 64 + (rand () & 31));
	while (count--)
		particle_new_random (pt_fallfadespark, part_tex_spark, org,
							 ofuzz * 0.75, 1, 96, r_realtime + 5,
							 ramp[rand () & 7], 255);
}

inline static void
R_BloodPuff (vec3_t org, int count)
{
	if (numparticles >= r_numparticles)
		return;

	particle_new (pt_bloodcloud, part_tex_smoke[rand () & 7], org, 9,
				  vec3_origin, r_realtime + 99, 68 + (rand () & 3), 128);
}

void
R_RunPuffEffect (vec3_t org, particle_effect_t type, byte count)
{
	if (!r_particles->int_val)
		return;

	switch (type) {
		case PE_GUNSHOT:
		{
			int scale = 16;

			if (count > 6)
				scale = 24;
			R_RunSparkEffect (org, count / 2, scale);
		}
			break;
		case PE_BLOOD:
			R_BloodPuff (org, count);
			break;
		case PE_LIGHTNINGBLOOD:
			R_BloodPuff (org, 5 + (rand () & 1));
			count = 4 + (rand () % 5);

			if (numparticles >= r_numparticles)
				return;
			else if (numparticles + count >= r_numparticles)
				count = r_numparticles - numparticles - 1;

			particle_new (pt_smokecloud, part_tex_smoke[rand () & 7], org,
						  3, vec3_origin, r_realtime + 9,
						  12 + (rand () & 3), 64 + (rand () & 31));
			while (count--)
				particle_new_random (pt_fallfadespark, part_tex_spark, org,
									 12, 2, 128, r_realtime + 5,
									 244 + (rand () % 3), 255);
			break;
		default:
			break;
	}
}

void
R_RunParticleEffect (vec3_t org, int color, int count)
{
	int         scale, i, j;
	int			k = count;
	vec3_t      porg;

	if (!r_particles->int_val)
		return;

	if (numparticles >= r_numparticles)
		return;

	if (count > 130)
		scale = 3;
	else if (count > 20)
		scale = 2;
	else
		scale = 1;

	if (numparticles + k >= r_numparticles)
		k = r_numparticles - numparticles;
	count = k;

	for (i = 0; i < count; i++) {
		for (j = 0; j < 3; j++) {
			porg[j] = org[j] + scale * ((rand () & 15) - 8);
		}
		particle_new (pt_grav, part_tex_dot, porg, 1.5, vec3_origin,
					  (r_realtime + 0.1 * (rand () % 5)),
					  (color & ~7) + (rand () & 7), 255);
	}
}

void
R_RunSpikeEffect (vec3_t org, particle_effect_t type)
{
	if (!r_particles->int_val)
		return;

	switch (type) {
		case PE_SPIKE:
			R_RunSparkEffect (org, 5, 8);
			break;
		case PE_SUPERSPIKE:
			R_RunSparkEffect (org, 10, 8);
			break;
		case PE_KNIGHTSPIKE:
			R_RunSparkEffect (org, 10, 8);
			break;
		case PE_WIZSPIKE:
			R_RunSparkEffect (org, 15, 16);
			break;
		default:
			break;
	}
}

void
R_LavaSplash (vec3_t org)
{
	float       vel;
	int         i, j;
	int			k = 256;
	vec3_t      dir, porg, pvel;

	if (!r_particles->int_val)
		return;

	if (numparticles + k >= r_numparticles) {
		return;
	} // else if (numparticles + k >= r_numparticles) {
//		k = r_numparticles - numparticles;
//	}

	for (i = -128; i < 128; i += 16) {
		for (j = -128; j < 128; j += 16) {
			dir[0] = j + (rand () & 7);
			dir[1] = i + (rand () & 7);
			dir[2] = 256;

			porg[0] = org[0] + dir[0];
			porg[1] = org[1] + dir[1];
			porg[2] = org[2] + (rand () & 63);

			VectorNormalize (dir);
			vel = 50 + (rand () & 63);
			VectorScale (dir, vel, pvel);
			particle_new (pt_grav, part_tex_dot, porg, 3, pvel,
						  (r_realtime + 2 + (rand () & 31) * 0.02),
						  (224 + (rand () & 7)), 193);
		}
	}
}

void
R_TeleportSplash (vec3_t org)
{
	float       vel;
	int         i, j, k;
	int			l = 896;
	vec3_t      dir, porg, pvel;

	if (!r_particles->int_val)
		return;

	if (numparticles + l >= r_numparticles) {
		return;
	} // else if (numparticles + l >= r_numparticles) {
//		l = r_numparticles - numparticles;
//	}

	for (i = -16; i < 16; i += 4)
		for (j = -16; j < 16; j += 4)
			for (k = -24; k < 32; k += 4) {
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				porg[0] = org[0] + i + (rand () & 3);
				porg[1] = org[1] + j + (rand () & 3);
				porg[2] = org[2] + k + (rand () & 3);

				VectorNormalize (dir);
				vel = 50 + (rand () & 63);
				VectorScale (dir, vel, pvel);
				particle_new (pt_grav, part_tex_spark, porg, 0.6, pvel,
							  (r_realtime + 0.2 + (rand () & 7) * 0.02),
							  (7 + (rand () & 7)), 255);
			}
}

void
R_RocketTrail (entity_t *ent)
{
	float		dist, len, pscale, pscalenext;
	vec3_t		subtract, vec;

	if (!r_particles->int_val)
		return;

	R_AddFire (ent->old_origin, ent->origin, ent);

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	pscale = 1.5 + qfrandom (1.5);

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		pscalenext = 1.5 + qfrandom (1.5);
		dist = (pscale + pscalenext) * 3.0;

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		// Misty-chan's Easter Egg: change color to (rand () & 255)
		particle_new (pt_smoke, part_tex_smoke[rand () & 7], ent->old_origin,
					  pscale, vec3_origin, r_realtime + 2.0,
					  12 + (rand () & 3), 128 + (rand () & 31));
		pscale = pscalenext;
	}
}

void
R_GrenadeTrail (entity_t *ent)
{
	float		dist, len, pscale, pscalenext;
	vec3_t		subtract, vec;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	pscale = 6.0 + qfrandom (7.0);

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		pscalenext = 6.0 + qfrandom (7.0);
		dist = (pscale + pscalenext) * 2.0;

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		// Misty-chan's Easter Egg: change color to (rand () & 255)
		particle_new (pt_smoke, part_tex_smoke[rand () & 7], ent->old_origin,
					  pscale, vec3_origin, r_realtime + 2.0, (rand () & 3),
					  128 + (rand () & 31));
		pscale = pscalenext;
	}
}

void
R_BloodTrail (entity_t *ent)
{
	float		dist, len, pscale, pscalenext;
	int			j;
	vec3_t		subtract, vec, porg, pvel;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	pscale = 5.0 + qfrandom (10.0);

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		VectorCopy (vec3_origin, pvel);
		VectorCopy (ent->old_origin, porg);

		pscalenext = 5.0 + qfrandom (10.0);
		dist = (pscale + pscalenext) * 1.5;

		for (j = 0; j < 3; j++) {
			pvel[j] = (qfrandom (24.0) - 12.0);
			porg[j] = ent->old_origin[j] + qfrandom (3.0) - 1.5;
		}

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		particle_new (pt_grav, part_tex_smoke[rand () & 7], porg, pscale, pvel,
					  r_realtime + 2.0, 68 + (rand () & 3), 255);
		pscale = pscalenext;
	}
}

void
R_SlightBloodTrail (entity_t *ent)
{
	float		dist, len, pscale, pscalenext;
	int			j;
	vec3_t      subtract, vec, porg, pvel;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	pscale = 1.5 + qfrandom (7.5);

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		VectorCopy (vec3_origin, pvel);
		VectorCopy (ent->old_origin, porg);

		pscalenext = 1.5 + qfrandom (7.5);
		dist = (pscale + pscalenext) * 1.5;

		for (j = 0; j < 3; j++) {
			pvel[j] = (qfrandom (12.0) - 6.0);
			porg[j] = ent->old_origin[j] + qfrandom (3.0) - 1.5;
		}

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		particle_new (pt_grav, part_tex_smoke[rand () & 7], porg, pscale, pvel,
					  r_realtime + 1.5, 68 + (rand () & 3), 192);
		pscale = pscalenext;
	}
}

void
R_GreenTrail (entity_t *ent)
{
	float		dist, len;
	static int	tracercount;
	vec3_t		subtract, vec, pvel;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	dist = 3.0;

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		VectorCopy (vec3_origin, pvel);
		tracercount++;
		if (tracercount & 1) {
			pvel[0] = 30 * vec[1];
			pvel[1] = 30 * -vec[0];
		} else {
			pvel[0] = 30 * -vec[1];
			pvel[1] = 30 * vec[0];
		}

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		particle_new (pt_fire, part_tex_smoke[rand () & 7], ent->old_origin,
					  2.0 + qfrandom (1.0), pvel, r_realtime + 0.5,
					  52 + (rand () & 4), 255);
	}
}

void
R_FlameTrail (entity_t *ent)
{
	float		dist, len;
	static int	tracercount;
	vec3_t		subtract, vec, pvel;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	dist = 3.0;

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		VectorCopy (vec3_origin, pvel);
		tracercount++;
		if (tracercount & 1) {
			pvel[0] = 30 * vec[1];
			pvel[1] = 30 * -vec[0];
		} else {
			pvel[0] = 30 * -vec[1];
			pvel[1] = 30 * vec[0];
		}

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		particle_new (pt_fire, part_tex_smoke[rand () & 7], ent->old_origin,
					  2.0 + qfrandom (1.0), pvel, r_realtime + 0.5, 234, 255);
	}
}

void
R_VoorTrail (entity_t *ent)
{
	float		dist, len;
	int			j;
	vec3_t		subtract, vec, porg;

	if (!r_particles->int_val)
		return;

	VectorSubtract (ent->origin, ent->old_origin, vec);
	len = VectorNormalize (vec);
	dist = 3.0;

	while (len > 0) {
		if (numparticles >= r_numparticles)
			return;

		for (j = 0; j < 3; j++)
			porg[j] = ent->old_origin[j] + qfrandom (16.0) - 8.0;

		VectorScale (vec, min(dist, len), subtract);
		VectorAdd (ent->old_origin, subtract, ent->old_origin);
		len -= dist;

		particle_new (pt_static, part_tex_dot, porg, 1.0 + qfrandom (1.0),
					  vec3_origin, r_realtime + 0.3,
					  9 * 16 + 8 + (rand () & 3), 255);
	}
}

void
R_DrawParticles (void)
{
	byte			i;
	unsigned char  *at;
	float			dvel, grav, fast_grav, minparticledist, scale;
	int				activeparticles, maxparticle, j, k;
	particle_t	   *part;
	vec3_t			up_scale, right_scale, up_right_scale, down_right_scale;
	
	if (!r_particles->int_val)
		return;

	// LordHavoc: particles should not affect zbuffer
	qfglDepthMask (GL_FALSE);

	varray[0].texcoord[0] = 0; varray[0].texcoord[1] = 1;
	varray[1].texcoord[0] = 0; varray[1].texcoord[1] = 0;
	varray[2].texcoord[0] = 1; varray[2].texcoord[1] = 0;
	varray[3].texcoord[0] = 1; varray[3].texcoord[1] = 1;

	grav = (fast_grav = r_frametime * 800) * 0.05;
	dvel = 4 * r_frametime;

	minparticledist = DotProduct (r_refdef.vieworg, vpn) + 32.0f;

	activeparticles = 0;
	maxparticle = -1;
	j = 0;

	for (k = 0, part = particles; k < numparticles; k++, part++) {
		// LordHavoc: this is probably no longer necessary, as it is
		// checked at the end, but could still happen on weird particle
		// effects, left for safety...
		if (part->die <= r_realtime) {
			freeparticles[j++] = part;
			continue;
		}
		maxparticle = k;
		activeparticles++;

		// Don't render particles too close to us.
		// Note, we must still do physics and such on them.
		if (!(DotProduct (part->org, vpn) < minparticledist)) {
			at = (byte *) & d_8to24table[(byte) part->color];

			varray[0].color[0] = at[0];
			varray[0].color[1] = at[1];
			varray[0].color[2] = at[2];
			varray[0].color[3] = part->alpha;

			memcpy(varray[1].color, varray[0].color, sizeof(varray[0].color));
			memcpy(varray[2].color, varray[0].color, sizeof(varray[0].color));
			memcpy(varray[3].color, varray[0].color, sizeof(varray[0].color));

			scale = part->scale;

			VectorScale    (vup,    scale, up_scale);
			VectorScale    (vright, scale, right_scale);

			VectorAdd      (right_scale, up_scale, up_right_scale);
			VectorSubtract (right_scale, up_scale, down_right_scale);

			VectorAdd      (part->org, up_right_scale,   varray[0].vertex);
			VectorAdd      (part->org, down_right_scale, varray[1].vertex);
			VectorSubtract (part->org, up_right_scale,   varray[2].vertex);
			VectorSubtract (part->org, down_right_scale, varray[3].vertex);

			qfglBindTexture (GL_TEXTURE_2D, part->tex);
			qfglDrawArrays (GL_QUADS, 0, 4);
		}

		for (i = 0; i < 3; i++)
			part->org[i] += part->vel[i] * r_frametime;

		switch (part->type) {
			case pt_static:
				break;
			case pt_blob:
				for (i = 0; i < 3; i++)
					part->vel[i] += part->vel[i] * dvel;
				part->vel[2] -= grav;
				break;
			case pt_blob2:
				for (i = 0; i < 2; i++)
					part->vel[i] -= part->vel[i] * dvel;
				part->vel[2] -= grav;
				break;
			case pt_grav:
				part->vel[2] -= grav;
				break;
			case pt_smoke:
				if ((part->alpha -= r_frametime * 100) < 1)
					part->die = -1;
				part->scale += r_frametime * 4;
//				part->org[2] += r_frametime * 30 - grav;
				break;
			case pt_smokecloud:
				if ((part->alpha -= r_frametime * 140) < 1)
				{
					part->die = -1;
					break;
				}
				part->scale += r_frametime * 50;
				part->org[2] += r_frametime * 30;
				break;
			case pt_bloodcloud:
				if ((part->alpha -= r_frametime * 65) < 1)
				{
					part->die = -1;
					break;
				}
				part->scale += r_frametime * 4;
				part->vel[2] -= grav;
				break;
			case pt_fadespark:
				if ((part->alpha -= r_frametime * 256) < 1)
					part->die = -1;
				part->vel[2] -= grav;
				break;
			case pt_fadespark2:
				if ((part->alpha -= r_frametime * 512) < 1)
					part->die = -1;
				part->vel[2] -= grav;
				break;
			case pt_fallfadespark:
				if ((part->alpha -= r_frametime * 256) < 1)
					part->die = -1;
				part->vel[2] -= fast_grav;
				break;
			case pt_fire:
				if ((part->alpha -= r_frametime * 32) < 1)
					part->die = -1;
				part->scale -= r_frametime * 2;
				break;
			default:
				Con_DPrintf ("unhandled particle type %d\n", part->type);
				break;
		}
	}
	k = 0;
	while (maxparticle >= activeparticles) {
		*freeparticles[k++] = particles[maxparticle--];
		while (maxparticle >= activeparticles && 
				particles[maxparticle].die <= r_realtime)
			maxparticle--;
	}
	numparticles = activeparticles;

	qfglColor3ubv (color_white);
	qfglDepthMask (GL_TRUE);
}
