// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Refresh/render internal state variables (global).


#ifndef __R_STATE_H__
#define __R_STATE_H__

// Need data structure definitions.
#include "doomtype.h"
#include "r_defs.h"
#include "r_data/sprites.h"

//
// Refresh internal data structures,
//	for rendering.
//

extern "C" int			viewwidth;
extern "C" int			viewheight;

//
// Lookup tables for map data.
//
extern TArray<spritedef_t> sprites;
extern DWORD NumStdSprites;

extern int				numvertexes;
extern vertex_t*		vertexes;
extern int				numvertexdatas;
extern vertexdata_t*		vertexdatas;

extern int				numsegs;
extern seg_t*			segs;

extern int				numsectors;
extern sector_t*		sectors;

extern int				numsubsectors;
extern subsector_t* 	subsectors;

extern int				numnodes;
extern node_t*			nodes;

extern int				numlines;
extern line_t*			lines;

extern int				numsides;
extern side_t*			sides;

extern int				numzones;
extern zone_t*			zones;

extern node_t * 		gamenodes;
extern int 				numgamenodes;

extern subsector_t * 	gamesubsectors;
extern int 				numgamesubsectors;


//
// POV data.
//
extern fixed_t			viewz;
extern angle_t			viewangle;

extern AActor*			camera;		// [RH] camera instead of viewplayer
extern sector_t*		viewsector;	// [RH] keep track of sector viewing from

extern angle_t			xtoviewangle[MAXWIDTH+1];
extern int				FieldOfView;

int R_FindSkin (const char *name, int pclass);	// [RH] Find a skin

#endif // __R_STATE_H__
