/*
	sv_progs.c

	Quick QuakeC server code

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
#include "string.h"
#endif
#ifdef HAVE_STRINGS_H
#include "strings.h"
#endif

#include "cmd.h"
#include "server.h"
#include "sv_progs.h"
#include "world.h"

sv_globals_t sv_globals;
sv_funcs_t sv_funcs;

int eval_alpha;
int eval_scale;
int eval_glowsize;
int eval_glowcolor;
int eval_colormod;

progs_t	    sv_pr_state;
cvar_t     *r_skyname;
cvar_t     *sv_progs;

func_t	EndFrame;
func_t	SpectatorConnect;
func_t	SpectatorDisconnect;
func_t	SpectatorThink;

static int reserved_edicts = MAX_CLIENTS;

void
FindEdictFieldOffsets (progs_t *pr)
{
	dfunction_t *f;

	if (pr == &sv_pr_state) {
		// Zoid, find the spectator functions
		SpectatorConnect = SpectatorThink = SpectatorDisconnect = 0;

		if ((f = ED_FindFunction (&sv_pr_state, "SpectatorConnect")) != NULL)
			SpectatorConnect = (func_t) (f - sv_pr_state.pr_functions);
		if ((f = ED_FindFunction (&sv_pr_state, "SpectatorThink")) != NULL)
			SpectatorThink = (func_t) (f - sv_pr_state.pr_functions);
		if ((f = ED_FindFunction (&sv_pr_state, "SpectatorDisconnect")) != NULL)
			SpectatorDisconnect = (func_t) (f - sv_pr_state.pr_functions);

		// 2000-01-02 EndFrame function by Maddes/FrikaC
		EndFrame = 0;
		if ((f = ED_FindFunction (&sv_pr_state, "EndFrame")) != NULL)
			EndFrame = (func_t) (f - sv_pr_state.pr_functions);

		eval_alpha = FindFieldOffset (&sv_pr_state, "alpha");
		eval_scale = FindFieldOffset (&sv_pr_state, "scale");
		eval_glowsize = FindFieldOffset (&sv_pr_state, "glow_size");
		eval_glowcolor = FindFieldOffset (&sv_pr_state, "glow_color");
		eval_colormod = FindFieldOffset (&sv_pr_state, "colormod");
	}
}

int
ED_Prune_Edict (progs_t *pr, edict_t *ent)
{
	if (((int) ((entvars_t*)&ent->v)->spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
		return 1;
	return 0;
}

void
ED_PrintEdicts_f (void)
{
	ED_PrintEdicts (&sv_pr_state);
}

/*
	ED_PrintEdict_f

	For debugging, prints a single edicy
*/
void
ED_PrintEdict_f (void)
{
	int         i;

	i = atoi (Cmd_Argv (1));
	Con_Printf ("\n EDICT %i:\n", i);
	ED_PrintNum (&sv_pr_state, i);
}

void
ED_Count_f (void)
{
	ED_Count (&sv_pr_state);
}

void
PR_Profile_f (void)
{
	PR_Profile (&sv_pr_state);
}

int
ED_Parse_Extra_Fields (progs_t *pr, char *key, char *value)
{
	/*
		If skyname is set, we want to allow skyboxes and set what the skybox
		name should be.  "qlsky" is supported since at least one other map
		uses it already.
	*/
	if (strcaseequal (key, "skyname")		// QuakeForge
		|| strcaseequal (key, "sky")		// Q2/DarkPlaces
		|| strcaseequal (key, "qlsky")) {	// QuakeLives
		Info_SetValueForKey (svs.info, "skybox", "1", MAX_SERVERINFO_STRING);
		Cvar_Set (r_skyname, value);
		return 1;
	}
	return 0;
}

void
SV_LoadProgs (void)
{
	PR_LoadProgs (&sv_pr_state, sv_progs->string);
	if (!sv_pr_state.progs)
		SV_Error ("SV_LoadProgs: couldn't load %s", sv_progs->string);
	if (sv_pr_state.progs->crc != PROGHEADER_CRC)
		SV_Error ("You must have the qwprogs.dat from QuakeWorld installed");
	// progs engine needs these globals anyway
	sv_globals.self = sv_pr_state.globals.self;
	sv_globals.time = sv_pr_state.globals.time;

	(void *) sv_globals.other = 
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "other")->ofs];
	(void *) sv_globals.world =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "world")->ofs];
	(void *) sv_globals.frametime =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "frametime")->ofs];
	(void *) sv_globals.newmis =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "newmis")->ofs];
	(void *) sv_globals.force_retouch =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "force_retouch")->ofs];
	(void *) sv_globals.mapname =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "mapname")->ofs];
	(void *) sv_globals.serverflags =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "serverflags")->ofs];
	(void *) sv_globals.total_secrets =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "total_secrets")->ofs];
	(void *) sv_globals.total_monsters =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "total_monsters")->ofs];
	(void *) sv_globals.found_secrets =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "found_secrets")->ofs];
	(void *) sv_globals.killed_monsters =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "killed_monsters")->ofs];
	(void *) sv_globals.parms =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "parm1")->ofs];
	(void *) sv_globals.v_forward =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "v_forward")->ofs];
	(void *) sv_globals.v_up =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "v_up")->ofs];
	(void *) sv_globals.v_right =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "v_right")->ofs];
	(void *) sv_globals.trace_allsolid =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_allsolid")->ofs];
	(void *) sv_globals.trace_startsolid =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_startsolid")->ofs];
	(void *) sv_globals.trace_fraction =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_fraction")->ofs];
	(void *) sv_globals.trace_endpos =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_endpos")->ofs];
	(void *) sv_globals.trace_plane_normal =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_plane_normal")->ofs];
	(void *) sv_globals.trace_plane_dist =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_plane_dist")->ofs];
	(void *) sv_globals.trace_ent =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_ent")->ofs];
	(void *) sv_globals.trace_inopen =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_inopen")->ofs];
	(void *) sv_globals.trace_inwater =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "trace_inwater")->ofs];
	(void *) sv_globals.msg_entity =
		&sv_pr_state.pr_globals[PR_FindGlobal (&sv_pr_state, "msg_entity")->ofs];

	sv_funcs.main =
		ED_FindFunction (&sv_pr_state, "main") - sv_pr_state.pr_functions;
	sv_funcs.StartFrame =
		ED_FindFunction (&sv_pr_state, "StartFrame") - sv_pr_state.pr_functions;
	sv_funcs.PlayerPreThink =
		ED_FindFunction (&sv_pr_state, "PlayerPreThink") - sv_pr_state.pr_functions;
	sv_funcs.PlayerPostThink =
		ED_FindFunction (&sv_pr_state, "PlayerPostThink") - sv_pr_state.pr_functions;
	sv_funcs.ClientKill =
		ED_FindFunction (&sv_pr_state, "ClientKill") - sv_pr_state.pr_functions;
	sv_funcs.ClientConnect =
		ED_FindFunction (&sv_pr_state, "ClientConnect") - sv_pr_state.pr_functions;
	sv_funcs.PutClientInServer =
		ED_FindFunction (&sv_pr_state, "PutClientInServer") - sv_pr_state.pr_functions;
	sv_funcs.ClientDisconnect =
		ED_FindFunction (&sv_pr_state, "ClientDisconnect") - sv_pr_state.pr_functions;
	sv_funcs.SetNewParms =
		ED_FindFunction (&sv_pr_state, "SetNewParms") - sv_pr_state.pr_functions;
	sv_funcs.SetChangeParms =
		ED_FindFunction (&sv_pr_state, "SetChangeParms") - sv_pr_state.pr_functions;
}

void
SV_Progs_Init (void)
{
	sv_pr_state.edicts = &sv.edicts;
	sv_pr_state.num_edicts = &sv.num_edicts;
	sv_pr_state.time = &sv.time;
	sv_pr_state.reserved_edicts = &reserved_edicts;
	sv_pr_state.unlink = SV_UnlinkEdict;
	sv_pr_state.flush = SV_FlushSignon;

	Cmd_AddCommand ("edict", ED_PrintEdict_f, "Report information on a given edict in the game. (edict (edict number))");
	Cmd_AddCommand ("edicts", ED_PrintEdicts_f, "Display information on all edicts in the game.");
	Cmd_AddCommand ("edictcount", ED_Count_f, "Display summary information on the edicts in the game.");
	Cmd_AddCommand ("profile", PR_Profile_f, "FIXME: Report information about QuakeC Stuff (???) No Description");
}

void
SV_Progs_Init_Cvars (void)
{
	r_skyname =
		Cvar_Get ("r_skyname", "", CVAR_SERVERINFO, "name of skybox");
	sv_progs = Cvar_Get ("sv_progs", "qwprogs.dat", CVAR_ROM,
						 "Allows selectable game progs if you have several "
						 "of them in the gamedir");
}
