// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	 Test executable function stubs
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <stdarg.h>

#include "actor.h"
#include "cmdlib.h"
#include "p_ctf.h"
#include "d_player.h"
#include "m_argv.h"
#include "s_sound.h"
#include "oscanner.h"
#include "c_dispatch.h"
#include "w_wad.h"

bool clientside = false; // don't want any rendering code called
bool serverside = true;
baseapp_t baseapp = test;

bool unnatural_level_progression = false;
bool step_mode = false;
wbstartstruct_t wminfo;

bool predicting;
int demostartgametic;
bool isFast;
int gametic;
gamestate_t gamestate;

CVAR_FUNC_IMPL (sv_allowwidescreen) {}
CVAR_FUNC_IMPL (sv_sharekeys) {}
CVAR_RANGE (sv_teamsinplay, "2", "Teams that are enabled", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 2.0f, 3.0f)
CVAR (sv_maxplayersperteam, "0", "Maximum number of players that can be on a team", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
CVAR (sv_maxplayers,		"0", "maximum players who can join the game, others are spectators", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)

void C_AddTabCommand(char const *) {}
void C_RemoveTabCommand(char const *) {}
void P_ShowSpawns(MapThing*) {}
void P_SpawnPlayer(player_t&, mapthing2_t*) {}
void G_DeathMatchSpawnPlayer(player_t&) {}
player_t& consoleplayer() { return idplayer(consoleplayer_id); }
player_t& displayplayer() { return idplayer(displayplayer_id); }

void BuildDefaultShademap(palette_t const *, shademap_t &) {}
palindex_t V_BestColor(const argb_t* palette_colors, int r, int g, int b) { return 0; }
palindex_t V_BestColor(const argb_t *palette_colors, argb_t color) { return 0; }

argb_t V_GetColorFromString(const std::string& str)
{
    return 0;
}

const palette_t* V_GetDefaultPalette()
{
	static palette_t default_palette;
	return &default_palette;
}

void D_SendServerInfoChange(const cvar_t *cvar, const char *value) {}
void D_DoServerInfoChange(byte **stream) {}
void D_WriteUserInfoStrings(int i, byte **stream, bool compact) {}
void D_ReadUserInfoStrings(int i, byte **stream, bool update) {}

void SV_SpawnMapMobj(AActor *mo) {}
void SV_SpawnMobj(AActor *mobj) {}
void SV_TouchSpecial(const AActor& special, player_t& player) {}
ItemEquipVal SV_FlagTouch (player_t &player, team_t f, bool firstgrab) { return IEV_NotEquipped; }
void SV_SocketTouch (player_t &player, team_t f) {}
void SV_SendKillMobj(const AActor *source, const AActor *target, const AActor *inflictor, bool joinkill) {}
void SV_SendRaiseMobj(const AActor* source, const AActor* corpse) {}
void SV_SendDamagePlayer(player_t *player, const AActor* inflictor, int healthDamage, int armorDamage) {}
void SV_SendDamageMobj(const AActor *target, int pain) {}
void SV_CTFEvent(team_t f, flag_score_t event, player_t &who) {}
void SV_UpdateFrags(player_t &player) {}
void SV_ActorTarget(const AActor *actor) {}
void SV_SendDestroyActor(const AActor *mo) {}
void SV_ExplodeMissile(const AActor *mo) {}
void SV_SendPlayerInfo(player_t &player) {}
void SV_PreservePlayer(player_t &player) {}
void SV_BroadcastSector(int sectornum) {}
void SV_UpdateMobj(const AActor* mo) {}
void SV_UpdateMobjState(const AActor* mo) {}

void CTF_RememberFlagPos(mapthing2_t *mthing) {}
void CTF_SpawnFlag(team_t f) {}
bool SV_AwarenessUpdate(player_t &pl, AActor* mo) { return true; }
void SV_SendPackets(void) {}
void SV_SendExecuteLineSpecial(byte special, line_t* line, AActor* activator, int arg0,
                               int arg1, int arg2, int arg3, int arg4) {}

void SV_UpdateMonsterRespawnCount() {}
void SV_Sound(const AActor* mo, byte channel, const char* name, byte attenuation) {}

void R_ExitLevel() {}
void D_SetupUserInfo (void) {}
void D_UserInfoChanged (cvar_t *cvar) {}

void PickupMessage(const AActor *toucher, const char *message) {}
void WeaponPickupMessage(const AActor *toucher, const weapontype_t &Weapon) {}

void AM_Stop(void) {}

void RefreshPalettes (void) {}

void V_RefreshColormaps() {}

void C_MidPrint (const char *msg, player_t *p, int msgtime) {}
size_t C_BasePrint(const int printlevel, const char* color_code, const std::string& str) { return 0; }

void S_NoiseDebug() {}
void S_Init(float sfxVolume, float musicVolume) {}
void S_Start() {}
void S_Stop() {}
void S_SoundID(int channel, int sound_id, float volume, int attenuation) {}
void S_SoundID(const AActor *ent, int channel, int sound_id, float volume, int attenuation) {}
void S_SoundID(const fixed_t *pt, int channel, int sound_id, float volume, int attenuation) {}
void S_LoopedSoundID(const AActor *ent, int channel, int sound_id, float volume, int attenuation) {}
void S_LoopedSoundID(const fixed_t *pt, int channel, int sound_id, float volume, int attenuation) {}
void S_PlatSound(const fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(int channel, const char *name, float volume, int attenuation) {}
void S_Sound(const AActor *ent, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(const fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_LoopedSound(const AActor *ent, int channel, const char *name, float volume, int attenuation) {}
void S_LoopedSound(const fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation) {}
void S_StopSound(const fixed_t *pt) {}
void S_StopSound(const fixed_t *pt, int channel) {}
void S_StopSound(const AActor *ent, int channel) {}
void S_StopAllChannels() {}
void S_RelinkSound(const AActor *from, const AActor *to) {}
bool S_GetSoundPlayingInfo(const fixed_t *pt, int sound_id) { return false; }
bool S_GetSoundPlayingInfo(const AActor *ent, int sound_id) {	return S_GetSoundPlayingInfo (ent ? &ent->x : NULL, sound_id); }
void S_PauseSound() {}
void S_ResumeSound() {}
void S_UpdateSounds(void *listener_p) {}
void S_UpdateMusic() {}
void S_SetMusicVolume(float volume) {}
void S_SetSfxVolume(float volume) {}
void S_StartMusic(std::string musicname) {}
void S_ChangeMusic(std::string musicname, bool looping, int order)  {}
void S_StopMusic() {}
void A_Ambient(AActor *actor) {}
void S_ActivateAmbient(AActor *origin, int ambient) {}

void AM_SetBaseColorDoom() {}
void AM_SetBaseColorRaven() {}
void AM_SetBaseColorStrife() {}

void G_DoLoadLevel(int) {}
void G_DeferedInitNew(const OLumpName&) {}
void G_DeferedFullReset() {}
void G_DeferedReset() {}
void G_ExitLevel(int,int,bool) {}
void G_SecretExitLevel(int,int,bool) {}
void D_Init() {}
void D_Shutdown() {}

FArchive &operator<< (FArchive &arc, UserInfo &info) { return arc; }
FArchive &operator>> (FArchive &arc, UserInfo &info) { return arc; }
void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
    const LineActivationType activationType, const bool bossaction) {}
void UV_SoundAvoidPlayer(const AActor *mo, byte channel, const char *name, byte attenuation) {}
void OnChangedSwitchTexture (line_t *line, int useAgain) {}

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
	int index = ang >> ANGLETOFINESHIFT;

	tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
	ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

#define R_P2ATHRESHOLD (INT_MAX / 4)

angle_t R_PointToAngle (fixed_t x, fixed_t y)
{
    return R_PointToAngle2 (viewx, viewy, x, y);
}

angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
	x -= viewx;
	y -= viewy;

	if((x | y) == 0)
		return 0;

	if(x < R_P2ATHRESHOLD && x > -R_P2ATHRESHOLD &&
		y < R_P2ATHRESHOLD && y > -R_P2ATHRESHOLD)
	{
		if(x >= 0)
		{
			if (y >= 0)
			{
				if(x > y)
				{
					// octant 0
					return tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 1
					return ANG90 - 1 - tantoangle_acc[SlopeDiv(x, y)];
				}
			}
			else // y < 0
			{
				y = -y;

				if(x > y)
				{
					// octant 8
					return 0 - tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 7
					return ANG270 + tantoangle_acc[SlopeDiv(x, y)];
				}
			}
		}
		else // x < 0
		{
			x = -x;

			if(y >= 0)
			{
				if(x > y)
				{
					// octant 3
					return ANG180 - 1 - tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 2
					return ANG90 + tantoangle_acc[SlopeDiv(x, y)];
				}
			}
			else // y < 0
			{
				y = -y;

				if(x > y)
				{
					// octant 4
					return ANG180 + tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 5
					return ANG270 - 1 - tantoangle_acc[SlopeDiv(x, y)];
				}
			}
		}
	}
	else
	{
      return (angle_t)(atan2((double)y, (double)x) * (ANG180 / PI));
	}

   return 0;
}

translationref_t::translationref_t() : m_table(NULL), m_player_id(-1) {}
translationref_t::translationref_t(const byte *table) : m_table(table), m_player_id(-1) {}
translationref_t::translationref_t(const byte *table, const int player_id) : m_table(table), m_player_id(player_id) {}

shaderef_t::shaderef_t() : m_colors(NULL), m_mapnum(-1), m_colormap(NULL), m_shademap(NULL) {}
shaderef_t::shaderef_t(const shademap_t * const colors, const int mapnum) : m_colors(colors), m_mapnum(mapnum)
{
	#if ODAMEX_DEBUG
	// NOTE(jsd): Arbitrary value picked here because we don't record the max number of colormaps for dynamic ones... or do we?
	if (m_mapnum >= 8192)
	{
		throw CFatalError(fmt::format("32bpp: shaderef_t::shaderef_t() called with mapnum = {}, which looks too large", m_mapnum));
	}
	#endif

	if (m_colors != NULL)
	{
		if (m_colors->colormap != NULL)
			m_colormap = m_colors->colormap + (256 * m_mapnum);
		else
			m_colormap = NULL;

		if (m_colors->shademap != NULL)
			m_shademap = m_colors->shademap + (256 * m_mapnum);
		else
			m_shademap = NULL;

		// Detect if the colormap is dynamic:
		m_dyncolormap = NULL;

		if (m_colors != &(V_GetDefaultPalette()->maps))
		{
			// Find the dynamic colormap by the `m_colors` pointer:
			extern dyncolormap_t NormalLight;
			dyncolormap_t *colormap = &NormalLight;

			do
			{
				if (m_colors == colormap->maps.m_colors)
				{
					m_dyncolormap = colormap;
					break;
				}
				colormap = colormap->next;
			} while (colormap);
		}
	}
	else
	{
		m_colormap = NULL;
		m_shademap = NULL;
		m_dyncolormap = NULL;
	}
}

dyncolormap_t NormalLight;

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb)
{
	argb_t color(lr, lg, lb);
	argb_t fade(fr, fg, fb);
	dyncolormap_t *colormap = &NormalLight;

	// Bah! Simple linear search because I want to get this done.
	while (colormap) {
		if (color == colormap->color && fade == colormap->fade)
			return colormap;
		else
			colormap = colormap->next;
	}

	// Not found. Create it.
	colormap = (dyncolormap_t *)Z_Malloc (sizeof(*colormap), PU_LEVEL, 0);
	shademap_t *maps = new shademap_t();
	maps->colormap = (byte *)Z_Malloc (NUMCOLORMAPS*256*sizeof(byte)+3+255, PU_LEVEL, 0);
	maps->colormap = (byte *)(((ptrdiff_t)maps->colormap + 255) & ~0xff);
	maps->shademap = (argb_t *)Z_Malloc (NUMCOLORMAPS*256*sizeof(argb_t)+3+255, PU_LEVEL, 0);
	maps->shademap = (argb_t *)(((ptrdiff_t)maps->shademap + 255) & ~0xff);

	colormap->maps = shaderef_t(maps, 0);
	colormap->color = color;
	colormap->fade = fade;
	colormap->next = NormalLight.next;
	NormalLight.next = colormap;

	// [AM] We don't keep the necessary palette info on the server to do this.
	//BuildColoredLights (maps, lr, lg, lb, fr, fg, fb);

	return colormap;
}

void CTF_CheckFlags (player_t &player)
{
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		if(player.flags[i])
		{
			player.flags[i] = false;
			GetTeamInfo((team_t)i)->FlagData.flagger = 0;
		}
	}
}

void R_InitSkyDefs() {}
void R_ClearSkyDefs() {}
bool R_IsSkyFlat(int flatnum)
{
	return false;
}

VERSION_CONTROL (test_stubs_cpp, "$Id$")
