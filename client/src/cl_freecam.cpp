// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   The freecam is a client-side only player that is injected
//	 during netdemo playback, lets you free roam and watch at your leis
//
//-----------------------------------------------------------------------------

#include "cl_freecam.h"

fixed_t Freecam::x = 0;
fixed_t Freecam::y = 0;
fixed_t Freecam::z = 0;
angle_t Freecam::angle = 0;
fixed_t Freecam::pitch = 0;
std::string Freecam::prevmap = "";

void Freecam::addFreecamPlayer()
{
	player_t* cam = &idplayer(freecam_id);

	if (Freecam::wipedOnLevelChange(cam))
	{
		Freecam::buildCam(cam);
	}

	if (cam->id != freecam_id && players.size() < MAXPLAYERS)  // initial add
	{
		cam = &players.emplace_back();
		cam->id = freecam_id;

		Freecam::buildCam(cam);
	}
}

void Freecam::buildCam(player_t* p_cam)
{
	AActor* mobj = new AActor(x, y, z, MT_PLAYER);
	mobj->player = p_cam;
	mobj->angle = angle;
	mobj->pitch = pitch;
	p_cam->camera = p_cam->mo = mobj->ptr();
	p_cam->viewz = z;
	p_cam->prevviewz = 1;

	// spec stuff
	p_cam->cheats |= CF_FLY;
	p_cam->mo->flags |= MF_NOCLIP;
	p_cam->playerstate = PST_LIVE;
	p_cam->spectator = true;
	p_cam->mo->oflags |= MFO_SPECTATOR;
	p_cam->mo->flags &= ~MF_SOLID;
	p_cam->mo->flags2 |= MF2_FLY;

	// used throughout code base to allow moving, etc
	p_cam->isNetdemoFreecam = true;
}

void Freecam::setStartPosition(fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
	Freecam::x = x;
	Freecam::y = y;
	Freecam::z = z;
	Freecam::angle = angle;
}

void Freecam::setPrevMap(std::string m)
{
	Freecam::prevmap = m;
}

void Freecam::savePosition()
{
	player_t* cam = &idplayer(freecam_id);

	if (cam->id == freecam_id && cam->isNetdemoFreecam)
	{
		x = cam->mo->x;
		y = cam->mo->y;
		z = cam->mo->z;
		angle = cam->mo->angle;
		pitch = cam->mo->pitch;
	}
}

bool Freecam::needPosition() 
{
	return (Freecam::x == 0 && Freecam::y == 0);
}

bool Freecam::wipedOnLevelChange(player_t* cam)
{
	return (cam->id == freecam_id && cam->isNetdemoFreecam && !cam->mo && !cam->camera);
}

void Freecam::reset()
{
	x = 0;
	y = 0;
	z = 0;
	angle = 0;
	pitch = 0;
}
