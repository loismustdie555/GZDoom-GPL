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
//      The automap module.


#ifndef __AMMAP_H__
#define __AMMAP_H__

struct event_t;
class FArchive;


void AM_StaticInit();

// Called by main loop.
bool AM_Responder (event_t* ev, bool last);

// Called by main loop.
void AM_Ticker (void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer (void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop (void);

void AM_NewResolution ();
void AM_ToggleMap ();
void AM_LevelInit ();
void AM_SerializeMarkers(FArchive &arc);


#endif
