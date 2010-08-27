/*
	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	See file, 'COPYING', for details.

	$Id$
*/

#ifndef qfbsp_bsp5_h
#define qfbsp_bsp5_h

#include "QF/mathlib.h"
#include "QF/bspfile.h"

typedef struct plane_s {
	vec3_t      normal;
	vec_t       dist;
	int         type;
} plane_t;

#define MAX_THREADS 4

#define ON_EPSILON 0.05
#define BOGUS_RANGE 18000

// the exact bounding box of the brushes is expanded some for the headnode
// volume.  is this still needed?
#define SIDESPACE 24

#define TEX_SKIP -1
#define TEX_HINT -2

typedef struct visfacet_s {
	struct visfacet_s *next;

	int         planenum;
	int         planeside;		// which side is the front of the face
	int         texturenum;
	int         contents[2];	// 0 = front side

	struct visfacet_s *original;// face on node
	int         outputnumber;	// valid only for original faces after
								// write surfaces
	qboolean    detail;			// is a detail face

	struct winding_s *points;
	int        *edges;
} face_t;

typedef struct surface_s {
	struct surface_s *next;
	struct surface_s *original;	// before BSP cuts it up
	int         planenum;
	int         outputplanenum;	// valid only after WriteSurfacePlanes
	vec3_t      mins, maxs;
	qboolean    onnode;			// true if surface has already been used
								// as a splitting node
	qboolean    has_detail;		// true if the surface has detail brushes
	qboolean    has_struct;		// true if the surface has non-detail
								// brushes
	face_t     *faces;			// links to all the faces on either side
								// of the surf
} surface_t;

// there is a node_t structure for every node and leaf in the bsp tree
#define PLANENUM_LEAF  -1

typedef struct node_s {
	vec3_t      mins,maxs;		// bounding volume, not just points inside

// information for decision nodes	
	int         planenum;		// -1 = leaf node	
	int         outputplanenum;	// valid only after WriteNodePlanes
	int         firstface;		// decision node only
	int         numfaces;		// decision node only
	struct node_s *children[2];	// valid only for decision nodes
	face_t     *faces;			// decision nodes only, list for both sides

// information for leafs
	int         contents;		// leaf nodes (0 for decision nodes)
	face_t    **markfaces;		// leaf nodes only, point to node faces
	struct portal_s *portals;
	int         visleafnum;		// -1 = solid
	int         valid;			// for flood filling
	int         occupied;		// light number in leaf for outside filling
	int         o_dist;			// distance to nearest entity
	int         detail;			// 1 if created by detail split
} node_t;




extern struct brushset_s *brushset;

void qprintf (const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));	// prints only if verbose

extern int  valid;

extern qboolean worldmodel;

// misc functions

face_t *AllocFace (void);
void FreeFace (face_t *f);

struct portal_s *AllocPortal (void);
void FreePortal (struct portal_s *p);

surface_t *AllocSurface (void);
void FreeSurface (surface_t *s);

node_t *AllocNode (void);
struct brush_s *AllocBrush (void);

//=============================================================================

extern bsp_t *bsp;

#endif//qfbsp_bsp5_h
