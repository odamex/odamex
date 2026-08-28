// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jess W
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
//   The freecam is a clientside-only player that is injected and used
//   with spynext to free roam the map during netdemos or allowed gamemodes
//
//-----------------------------------------------------------------------------

#include "cl_freecam.h"
#include "g_gametype.h"
#include "p_local.h"

EXTERN_CVAR (cl_noclip_spectator)

fixed_t cam_x = 0;
fixed_t cam_y = 0;
fixed_t cam_z = 0;
angle_t cam_angle = 0;
fixed_t cam_pitch = 0;

std::string Freecam::prevmap = "";

void Freecam::addFreecamPlayer()
{
	player_t* cam = &idplayer(freecamplayer_id);

	if (Freecam::wipedOnLevelChange(cam))
	{
		Freecam::buildCam(cam);
	}

	if (cam->id != freecamplayer_id && players.size() < MAXPLAYERS) // initial add
	{
		cam = &players.emplace_back();
		cam->id = freecamplayer_id;

		Freecam::buildCam(cam);
	}
}

bool Freecam::wipedOnLevelChange(player_t* cam)
{
	return (cam->id == freecamplayer_id && cam->isFreecam && not cam->mo && not cam->camera);
}

void Freecam::buildCam(player_t* p_cam)
{
	AActor* mobj = new AActor(cam_x, cam_y, cam_z, MT_PLAYER);
	mobj->player = p_cam;
	mobj->angle = cam_angle;
	mobj->pitch = cam_pitch;
	p_cam->camera = p_cam->mo = mobj->ptr();
	p_cam->prevviewz = 1;
	p_cam->viewz = 1;
	p_cam->viewheight = VIEWHEIGHT;

	// spec stuff
	p_cam->cheats |= CF_FLY;
	p_cam->spectator = true;
	p_cam->mo->oflags |= MFO_SPECTATOR;
	p_cam->mo->flags &= ~MF_SOLID;

	if (cl_noclip_spectator)
	{
		p_cam->cheats |= CF_NOCLIP;
	}

	// player.ingame() should always be false for the freecam
	p_cam->playerstate = PST_FREECAM;  

	p_cam->isFreecam = true;
}

void Freecam::setStartPosition()
{
	mapthing2_t* start = nullptr;

	if (not playerstarts.empty())
	{
		start = &playerstarts.front();
	}
	else if (not DeathMatchStarts.empty())
	{
		start = &DeathMatchStarts.front();
	}
	else
	{
		for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
		{
			TeamInfo* teamInfo = GetTeamInfo(static_cast<team_t>(iTeam));

			if (not teamInfo->Starts.empty())
			{
				start = &teamInfo->Starts.front();
				break;
			}
		}
	}

	if (start)
	{
		cam_x = start->x << FRACBITS;
		cam_y = start->y << FRACBITS;
		cam_z = ONFLOORZ;
		cam_angle = ANG45 * (start->angle / 45);
	}
}

void Freecam::moveToDeathSpot(fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
	player_t* cam = &idplayer(freecamplayer_id);

	if (cam->id == freecamplayer_id && cam->isFreecam)
	{
		cam->mo->x = x;
		cam->mo->y = y;
		cam->mo->z = z;
		cam->mo->angle = angle;
		cam->mo->pitch = 0;
		cam->viewheight = VIEWHEIGHT;
	}
}

void Freecam::savePosition()
{
	player_t* cam = &idplayer(freecamplayer_id);

	if (cam->id == freecamplayer_id && cam->isFreecam)
	{
		cam_x = cam->mo->x;
		cam_y = cam->mo->y;
		cam_z = cam->mo->z;
		cam_angle = cam->mo->angle;
		cam_pitch = cam->mo->pitch;
	}
}

bool Freecam::needPosition() 
{
	return (cam_x == 0 && cam_y == 0);
}

void Freecam::reset()
{
	cam_x = cam_y = cam_z = cam_angle = cam_pitch = 0;
}

bool Freecam::allowAdd()
{
	return (netdemo.isPlaying() || (G_IsLivesGame() && not G_IsTeamGame()));
}

bool Freecam::allowSpy()
{
	return (netdemo.isInPlayback() || 
			(consoleplayer().playerstate == PST_DEAD 
				&& consoleplayer().lives < 1 
				&& ::levelstate.getState() == LevelState::INGAME));
}
    
// a real 255th player connected (CL_UserInfo) and is taking the freecam spot
void Freecam::retireFor255thPlayer(player_t* cam)
{
	cam->isFreecam = false;
	cam->spectator = false;
	cam->cheats = 0;
	cam->playerstate = PST_LIVE;
	cam->prevviewz = 1;
	cam->mo->Destroy();
	cam->camera->Destroy();
}
