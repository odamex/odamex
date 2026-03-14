// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//      Doom-family status bar code.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "st_stuff.h"
#include "i_video.h"
#include "m_random.h"
#include "st_lib.h"
#include "am_map.h"
#include "m_cheat.h"
#include "s_sound.h"
#include "gstrings.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "c_console.h"
#include "gi.h"
#include "g_gametype.h"
#include "p_ctf.h"

enum st_stateenum_t
{
	AutomapState,
	FirstPersonState
};

// N/256*100% probability that the normal face state will change.
#define ST_FACEPROBABILITY		96
// Location of status bar face.
#define ST_FX					(143)
#define ST_FY					(0)
// Should be set to patch width for tall numbers later on.
#define ST_TALLNUMWIDTH 		(tallnum[0]->width)
// Number of status faces.
#define ST_NUMPAINFACES 		5
#define ST_NUMSTRAIGHTFACES 	3
#define ST_NUMTURNFACES 		2
#define ST_NUMSPECIALFACES		3
#define ST_FACESTRIDE \
		(ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES)
#define ST_NUMEXTRAFACES		2
#define ST_NUMFACES \
		(ST_FACESTRIDE * ST_NUMPAINFACES + ST_NUMEXTRAFACES)
#define ST_TURNOFFSET			(ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET			(ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET		(ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET		(ST_EVILGRINOFFSET + 1)
#define ST_GODFACE				(ST_NUMPAINFACES * ST_FACESTRIDE)
#define ST_DEADFACE 			(ST_GODFACE + 1)
#define ST_FACESX				(143)
#define ST_FACESY				(0)
#define ST_EVILGRINCOUNT		(2 * TICRATE)
#define ST_STRAIGHTFACECOUNT	(TICRATE / 2)
#define ST_TURNCOUNT			(1 * TICRATE)
#define ST_OUCHCOUNT			(1 * TICRATE)
#define ST_RAMPAGEDELAY 		(2 * TICRATE)
#define ST_MUCHPAIN 			20

// A 320x32 status bar layout.
#define ST_AMMOWIDTH			3
#define ST_AMMOX				(44)
#define ST_AMMOY				(3)
#define ST_HEALTHWIDTH			3
#define ST_HEALTHX				(90)
#define ST_HEALTHY				(3)
#define ST_ARMSX				(111)
#define ST_ARMSY				(4)
#define ST_ARMSBGX				(104)
#define ST_ARMSBGY				(0)
#define ST_ARMSXSPACE			12
#define ST_ARMSYSPACE			10
#define ST_FLAGSBGX				(106)
#define ST_FLAGSBGY				(0)
#define ST_FRAGSX				(138)
#define ST_FRAGSY				(3)
#define ST_FRAGSWIDTH			2
#define ST_ARMORWIDTH			3
#define ST_ARMORX				(221)
#define ST_ARMORY				(3)
#define ST_FLGBOXX				(236)
#define ST_FLGBOXY				(0)
#define ST_FLGBOXBLUX			(239)
#define ST_FLGBOXBLUY			(3)
#define ST_FLGBOXREDX			(239)
#define ST_FLGBOXREDY			(18)
#define ST_KEY0WIDTH			8
#define ST_KEY0HEIGHT			5
#define ST_KEY0X				(239)
#define ST_KEY0Y				(3)
#define ST_KEY1WIDTH			ST_KEY0WIDTH
#define ST_KEY1X				(239)
#define ST_KEY1Y				(13)
#define ST_KEY2WIDTH			ST_KEY0WIDTH
#define ST_KEY2X				(239)
#define ST_KEY2Y				(23)
#define ST_AMMO0WIDTH			3
#define ST_AMMO0HEIGHT			6
#define ST_AMMO0X				(288)
#define ST_AMMO0Y				(5)
#define ST_AMMO1WIDTH			ST_AMMO0WIDTH
#define ST_AMMO1X				(288)
#define ST_AMMO1Y				(11)
#define ST_AMMO2WIDTH			ST_AMMO0WIDTH
#define ST_AMMO2X				(288)
#define ST_AMMO2Y				(23)
#define ST_AMMO3WIDTH			ST_AMMO0WIDTH
#define ST_AMMO3X				(288)
#define ST_AMMO3Y				(17)
#define ST_MAXAMMO0WIDTH		3
#define ST_MAXAMMO0HEIGHT		5
#define ST_MAXAMMO0X			(314)
#define ST_MAXAMMO0Y			(5)
#define ST_MAXAMMO1WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X			(314)
#define ST_MAXAMMO1Y			(11)
#define ST_MAXAMMO2WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X			(314)
#define ST_MAXAMMO2Y			(23)
#define ST_MAXAMMO3WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X			(314)
#define ST_MAXAMMO3Y			(17)

static int st_msgcounter = 0;
static st_stateenum_t st_gamestate;
static bool st_statusbaron;
static bool st_chat;
static bool st_oldchat;
static bool st_cursoron;
static lumpHandle_t sbar;
static short sbar_width = 0;

lumpHandle_t tallnum[10];
static lumpHandle_t tallpercent;
static lumpHandle_t shortnum[10];

lumpHandle_t keys[NUMCARDS + NUMCARDS / 2];
lumpHandle_t faces[ST_NUMFACES];
lumpHandle_t negminus;

static lumpHandle_t faceback;
static lumpHandle_t faceclassic[4];
static lumpHandle_t armsbg;
static lumpHandle_t flagsbg;
static lumpHandle_t arms[6][2];

static StatusBarWidgetNumber w_ready;
static StatusBarWidgetNumber w_frags;
static StatusBarWidgetPercent w_health;
static StatusBarWidgetMultiIcon w_arms[6];
static StatusBarWidgetMultiIcon w_faces;
static StatusBarWidgetMultiIcon w_keyboxes[3];
static StatusBarWidgetPercent w_armor;
static StatusBarWidgetNumber w_ammo[4];
static StatusBarWidgetNumber w_maxammo[4];
static StatusBarWidgetNumber w_lives;
static int st_fragscount;
static int st_oldhealth = -1;
static bool oldweaponsowned[NUMWEAPONS + 1];
static int st_facecount = 0;
int st_faceindex = 0;
static int keyboxes[3];
static int st_health, st_armor;
static int st_ammo[4], st_maxammo[4];
static int st_weaponowned[6] = {0}, st_current_ammo;
static int st_lives;
static int st_randomnumber;

extern bool st_needrefresh;

// these are now in d_dehacked.cpp
extern byte cheat_mus_seq[9];
extern byte cheat_choppers_seq[11];
extern byte cheat_god_seq[6];
extern byte cheat_ammo_seq[6];
extern byte cheat_ammonokey_seq[5];
extern byte cheat_noclip_seq[11];
extern byte cheat_commercial_noclip_seq[7];
extern byte cheat_powerup_seq[7][10];
extern byte cheat_clev_seq[10];
extern byte cheat_mypos_seq[8];

// functions in st_new.c
void ST_initNew();
void ST_unloadNew();

static byte CheatNoclip[] = {'i', 'd', 's', 'p', 'i', 's', 'p', 'o', 'p', 'd', 255};
static byte CheatNoclip2[] = {'i', 'd', 'c', 'l', 'i', 'p', 255};
static byte CheatMus[] = {'i', 'd', 'm', 'u', 's', 0, 0, 255};
static byte CheatChoppers[] = {'i', 'd', 'c', 'h', 'o', 'p', 'p', 'e', 'r', 's', 255};
static byte CheatGod[] = {'i', 'd', 'd', 'q', 'd', 255};
static byte CheatAmmo[] = {'i', 'd', 'k', 'f', 'a', 255};
static byte CheatAmmoNoKey[] = {'i', 'd', 'f', 'a', 255};
static byte CheatClev[] = {'i', 'd', 'c', 'l', 'e', 'v', 0, 0, 255};
static byte CheatMypos[] = {'i', 'd', 'm', 'y', 'p', 'o', 's', 255};
static byte CheatAmap[] = {'i', 'd', 'd', 't', 255};

static byte CheatPowerup[7][10] = {{'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'v', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 's', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'i', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'r', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'a', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 'l', 255},
                                   {'i', 'd', 'b', 'e', 'h', 'o', 'l', 'd', 255}};

cheatseq_t DoomCheats[] = {
    {CheatMus, 0, 1, 0, {0, 0}, cheat::ChangeMusic},
    {CheatPowerup[6], 0, 1, 0, {0, 0}, cheat::BeholdMenu},
    {CheatMypos, 0, 1, 0, {0, 0}, cheat::IdMyPos},
    {CheatAmap, 0, 0, 0, {0, 0}, cheat::AutoMap},
    {CheatGod, 0, 0, 0, {CHT_IDDQD, 0}, cheat::SetGeneric},
    {CheatAmmo, 0, 0, 0, {CHT_IDKFA, 0}, cheat::SetGeneric},
    {CheatAmmoNoKey, 0, 0, 0, {CHT_IDFA, 0}, cheat::SetGeneric},
    {CheatNoclip, 0, 0, 0, {CHT_NOCLIP, 0}, cheat::SetGeneric},
    {CheatNoclip2, 0, 0, 0, {CHT_NOCLIP, 1}, cheat::SetGeneric},
    {CheatPowerup[0], 0, 0, 0, {CHT_BEHOLDV, 0}, cheat::SetGeneric},
    {CheatPowerup[1], 0, 0, 0, {CHT_BEHOLDS, 0}, cheat::SetGeneric},
    {CheatPowerup[2], 0, 0, 0, {CHT_BEHOLDI, 0}, cheat::SetGeneric},
    {CheatPowerup[3], 0, 0, 0, {CHT_BEHOLDR, 0}, cheat::SetGeneric},
    {CheatPowerup[4], 0, 0, 0, {CHT_BEHOLDA, 0}, cheat::SetGeneric},
    {CheatPowerup[5], 0, 0, 0, {CHT_BEHOLDL, 0}, cheat::SetGeneric},
    {CheatChoppers, 0, 0, 0, {CHT_CHAINSAW, 0}, cheat::SetGeneric},
    {CheatClev, 0, 0, 0, {0, 0}, cheat::ChangeLevel}};

EXTERN_CVAR(sv_allowcheats)
EXTERN_CVAR(sv_allowredscreen)
EXTERN_CVAR(sv_allowfov)
EXTERN_CVAR(chasedemo)
EXTERN_CVAR(st_scale)

short ST_DoomBaseWidth()
{
	return sbar_width;
}

bool ST_DoomResponder(event_t* ev)
{
	bool eat = false;

	if (ev->type == ev_keyup && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
	{
		switch (ev->data1)
		{
		case AM_MSGENTERED:
			st_gamestate = AutomapState;
			ST_ForceRefresh();
			break;

		case AM_MSGEXITED:
			st_gamestate = FirstPersonState;
			break;
		}
	}
	else if (ev->type == ev_keydown && ev->data3)
	{
		for (auto& cheat : DoomCheats)
		{
			if (cheat::AddKey(&cheat, (byte)ev->data1, &eat))
			{
				if (cheat.DontCheck || cheat::AreCheatsEnabled())
				{
					eat |= cheat.Handler(&cheat);
				}
			}
		}
	}

	return eat;
}

BEGIN_COMMAND(god)
{
	if (!cheat::AreCheatsEnabled())
		return;

	cheat::DoCheat(consoleplayer(), CHT_GOD);
	CL_SendCheat(CHT_GOD);
}
END_COMMAND(god)

BEGIN_COMMAND(notarget)
{
	if (!cheat::AreCheatsEnabled())
		return;

	cheat::DoCheat(consoleplayer(), CHT_NOTARGET);
	CL_SendCheat(CHT_NOTARGET);
}
END_COMMAND(notarget)

BEGIN_COMMAND(fly)
{
	if (!consoleplayer().spectator && !cheat::AreCheatsEnabled())
		return;

	cheat::DoCheat(consoleplayer(), CHT_FLY);

	if (!consoleplayer().spectator)
	{
		CL_SendCheat(CHT_FLY);
	}
}
END_COMMAND(fly)

BEGIN_COMMAND(noclip)
{
	if (!cheat::AreCheatsEnabled())
		return;

	cheat::DoCheat(consoleplayer(), CHT_NOCLIP);
	CL_SendCheat(CHT_NOCLIP);
}
END_COMMAND(noclip)

BEGIN_COMMAND(chase)
{
	if (demoplayback)
	{
		if (chasedemo)
		{
			chasedemo.Set(0.0f);
			for (auto& player : players)
				player.cheats &= ~CF_CHASECAM;
		}
		else
		{
			chasedemo.Set(1.0f);
			for (auto& player : players)
				player.cheats |= CF_CHASECAM;
		}
	}
	else
	{
		if (!cheat::AreCheatsEnabled())
			return;

		cheat::DoCheat(consoleplayer(), CHT_CHASECAM);
	}
}
END_COMMAND(chase)

BEGIN_COMMAND(idmus)
{
	if (argc > 1)
	{
		OLumpName map;
		if (gameinfo.flags & GI_MAPxx)
		{
			const int l = atoi(argv[1]);
			if (l <= 99)
				map = CalcMapName(0, l);
			else
			{
				PrintFmt(PRINT_HIGH, "{}\n", GStrings(STSTR_NOMUS));
				return;
			}
		}
		else
		{
			map = CalcMapName(argv[1][0] - '0', argv[1][1] - '0');
		}

		level_pwad_info_t& info = getLevelInfos().findByName(map);
		if (level.levelnum != 0)
		{
			if (info.music[0])
			{
				S_ChangeMusic(std::string(info.music.c_str(), 8), 1);
				PrintFmt(PRINT_HIGH, "{}\n", GStrings(STSTR_MUS));
			}
		}
		else
		{
			PrintFmt(PRINT_HIGH, "{}\n", GStrings(STSTR_NOMUS));
		}
	}
}
END_COMMAND(idmus)

BEGIN_COMMAND(give)
{
	if (!cheat::AreCheatsEnabled())
		return;

	if (argc < 2)
		return;

	const std::string name = C_ArgCombine(argc - 1, (const char**)(argv + 1));
	if (name.length())
	{
		cheat::GiveTo(consoleplayer(), name.c_str());
		CL_SendGiveCheat(name.c_str());
	}
}
END_COMMAND(give)

BEGIN_COMMAND(fov)
{
	if (multiplayer && !sv_allowfov && (!cheat::AreCheatsEnabled() || !m_Instigator))
		return;

	if (argc != 2)
		PrintFmt(PRINT_HIGH, "FOV is {:g}\n", m_Instigator->player->fov);
	else
	{
		m_Instigator->player->fov = clamp((float)atof(argv[1]), 45.0f, 135.0f);
		R_ForceViewWindowResize();
	}
}
END_COMMAND(fov)

BEGIN_COMMAND(buddha)
{
	if (!cheat::AreCheatsEnabled())
		return;

	cheat::DoCheat(consoleplayer(), CHT_BUDDHA);
	CL_SendCheat(CHT_BUDDHA);
}
END_COMMAND(buddha)

static int ST_calcPainOffset()
{
	static int lastcalc;
	static int oldhealth = -1;

	const int health = clamp(displayplayer().health, -1, 100);

	if (health != oldhealth)
	{
		lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
		oldhealth = health;
	}

	return lastcalc;
}

static void ST_updateFaceWidget()
{
	static int lastattackdown = -1;
	static int priority = 0;
	player_t* plyr = &displayplayer();
	int i;

	if (priority < 10)
	{
		if (!plyr->health)
		{
			priority = 9;
			st_faceindex = ST_DEADFACE;
			st_facecount = 1;
		}
	}

	if (priority < 9)
	{
		if (plyr->bonuscount)
		{
			bool doevilgrin = false;

			for (i = 0; i < NUMWEAPONS; i++)
			{
				if (oldweaponsowned[i] != plyr->weaponowned[i])
				{
					doevilgrin = true;
					oldweaponsowned[i] = plyr->weaponowned[i];
				}
			}
			if (doevilgrin)
			{
				priority = 8;
				st_facecount = ST_EVILGRINCOUNT;
				st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
			}
		}
	}

	if (priority < 8)
	{
		if (plyr->damagecount && plyr->attacker && plyr->attacker != plyr->mo)
		{
			priority = 7;

			if (st_oldhealth - plyr->health > ST_MUCHPAIN)
			{
				st_facecount = ST_TURNCOUNT;
				st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
			}
			else
			{
				angle_t diffang;
				angle_t badguyangle = R_PointToAngle2(
				    plyr->mo->x, plyr->mo->y, plyr->attacker->x, plyr->attacker->y);

				if (badguyangle > plyr->mo->angle)
				{
					diffang = badguyangle - plyr->mo->angle;
					i = diffang > ANG180;
				}
				else
				{
					diffang = plyr->mo->angle - badguyangle;
					i = diffang <= ANG180;
				}

				st_facecount = ST_TURNCOUNT;
				st_faceindex = ST_calcPainOffset();

				if (diffang < ANG45)
				{
					st_faceindex += ST_RAMPAGEOFFSET;
				}
				else if (i)
				{
					st_faceindex += ST_TURNOFFSET;
				}
				else
				{
					st_faceindex += ST_TURNOFFSET + 1;
				}
			}
		}
	}

	if (priority < 7)
	{
		if (plyr->damagecount)
		{
			if (st_oldhealth - plyr->health > ST_MUCHPAIN)
			{
				priority = 7;
				st_facecount = ST_TURNCOUNT;
				st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
			}
			else
			{
				priority = 6;
				st_facecount = ST_TURNCOUNT;
				st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
			}
		}
	}

	if (priority < 6)
	{
		if (plyr->attackdown)
		{
			if (lastattackdown == -1)
				lastattackdown = ST_RAMPAGEDELAY;
			else if (!--lastattackdown)
			{
				priority = 5;
				st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
				st_facecount = 1;
				lastattackdown = 1;
			}
		}
		else
		{
			lastattackdown = -1;
		}
	}

	if (priority < 5)
	{
		if ((plyr->cheats & CF_GODMODE) || plyr->powers[pw_invulnerability])
		{
			priority = 4;
			st_faceindex = ST_GODFACE;
			st_facecount = 1;
		}
	}

	if (!st_facecount)
	{
		st_faceindex = ST_calcPainOffset() + (st_randomnumber % 3);
		st_facecount = ST_STRAIGHTFACECOUNT;
		priority = 0;
	}

	st_facecount--;
}

static void ST_updateWidgets()
{
	const player_t* plyr = &displayplayer();

	if (weaponinfo[plyr->readyweapon].ammotype == am_noammo)
		st_current_ammo = ST_DONT_DRAW_NUM;
	else
		st_current_ammo = plyr->ammo[weaponinfo[plyr->readyweapon].ammotype];

	st_health = plyr->health;
	st_armor = plyr->armorpoints;

	for (int i = 0; i < 4; i++)
	{
		st_ammo[i] = plyr->ammo[i];
		st_maxammo[i] = plyr->maxammo[i];
	}

	for (int i = 0; i < 6; i++)
	{
		st_weaponowned[i] = plyr->weaponowned[i + 1] ? 1 : 0;
	}

	for (int i = 0; i < 3; i++)
	{
		keyboxes[i] = plyr->cards[i] ? i : -1;

		if (plyr->cards[i + 3])
			keyboxes[i] = (keyboxes[i] == -1) ? i + 3 : i + 6;
	}

	ST_updateFaceWidget();

	if (sv_gametype == GM_CTF)
		st_fragscount = GetTeamInfo(plyr->userinfo.team)->Points;
	else
		st_fragscount = plyr->fragcount;

	if (G_IsLivesGame())
		st_lives = plyr->lives;
	else
		st_lives = ST_DONT_DRAW_NUM;

	if (!--st_msgcounter)
		st_chat = st_oldchat;
}

static void ST_UpdateSurfaceBpp()
{
	if (!stbar_surface || !stnum_surface)
		return;

	int currentbpp = screen->getSurface()->getBitsPerPixel();
	int stnumbpp = stnum_surface->getBitsPerPixel();
	int stbarbpp = stbar_surface->getBitsPerPixel();

	if (stbarbpp != currentbpp)
	{
		I_FreeSurface(stbar_surface);
		stbar_surface = I_AllocateSurface(sbar_width, 32, currentbpp);
	}

	if (stnumbpp != currentbpp)
	{
		I_FreeSurface(stnum_surface);
		stnum_surface = I_AllocateSurface(sbar_width, 32, currentbpp);
	}
}

static void ST_drawWidgets(bool force_refresh)
{
	w_ready.update(force_refresh);

	for (int i = 0; i < 4; i++)
	{
		w_ammo[i].update(force_refresh);
		w_maxammo[i].update(force_refresh);
	}

	w_health.update(force_refresh);
	w_armor.update(force_refresh);

	if (G_IsCoopGame())
	{
		for (int i = 0; i < 6; i++)
			w_arms[i].update(force_refresh);
	}

	w_faces.update(force_refresh);

	for (int i = 0; i < 3; i++)
		w_keyboxes[i].update(force_refresh);

	if (!G_IsCoopGame())
		w_frags.update(force_refresh);

	if (G_IsLivesGame())
		w_lives.update(true);
}

static void ST_refreshBackground()
{
	const IWindowSurface* surface = R_GetRenderingSurface();
	const int surface_width = surface->getWidth();
	const int surface_height = surface->getHeight();
	int scaled_x = (sbar_width - 320) / 2;

	if (surface_width > ST_WIDTH)
	{
		R_DrawBorder(0, ST_Y, ST_X, surface_height);
		R_DrawBorder(surface_width - ST_X, ST_Y, surface_width, surface_height);
	}

	stbar_surface->lock();

	const DCanvas* stbar_canvas = stbar_surface->getDefaultCanvas();
	stbar_canvas->DrawPatch(W_ResolvePatchHandle(sbar), 0, 0);

	if (sv_gametype == GM_CTF)
	{
		stbar_canvas->DrawPatch(W_ResolvePatchHandle(flagsbg), ST_FLAGSBGX + scaled_x, ST_FLAGSBGY);
	}
	else if (G_IsCoopGame())
	{
		stbar_canvas->DrawPatch(W_ResolvePatchHandle(armsbg), ST_ARMSBGX + scaled_x, ST_ARMSBGY);
	}

	if (multiplayer)
	{
		if (!demoplayback)
		{
			V_ColorMap =
			    translationref_t(translationtables + displayplayer_id * 256, displayplayer_id);
			stbar_canvas->DrawTranslatedPatch(W_ResolvePatchHandle(faceback), ST_FX + scaled_x, ST_FY);
		}
		else
		{
			stbar_canvas->DrawPatch(W_ResolvePatchHandle(faceclassic[displayplayer_id - 1]),
			                        ST_FX + scaled_x, ST_FY);
		}
	}

	stbar_surface->unlock();
}

static lumpHandle_t LoadFaceGraphic(const OLumpName& name)
{
	int lump = W_GetNumForName(name, ns_global);
	return W_CachePatchHandle(lump, PU_STATIC);
}

static void ST_loadGraphics()
{
	OLumpName namebuf;

	for (int i = 0; i < 10; i++)
	{
		namebuf = fmt::format("STTNUM{}", i);
		tallnum[i] = W_CachePatchHandle(namebuf, PU_STATIC);

		namebuf = fmt::format("STYSNUM{}", i);
		shortnum[i] = W_CachePatchHandle(namebuf, PU_STATIC);
	}

	tallpercent = W_CachePatchHandle("STTPRCNT", PU_STATIC);
	negminus = W_CachePatchHandle("STTMINUS", PU_STATIC);

	for (int i = 0; i < NUMCARDS + NUMCARDS / 2; i++)
	{
		namebuf = fmt::format("STKEYS{}", i);
		keys[i] = W_CachePatchHandle(namebuf, PU_STATIC);
	}

	armsbg = W_CachePatchHandle("STARMS", PU_STATIC);
	flagsbg = W_CachePatchHandle("STFLAGS", PU_STATIC);

	for (int i = 0; i < 6; i++)
	{
		namebuf = fmt::format("STGNUM{}", i + 2);
		arms[i][0] = W_CachePatchHandle(namebuf, PU_STATIC);
		arms[i][1] = shortnum[i + 2];
	}

	faceback = W_CachePatchHandle("STFBANY", PU_STATIC);

	for (int i = 0; i < 4; i++)
	{
		namebuf = fmt::format("STFB{}", i);
		faceclassic[i] = W_CachePatchHandle(namebuf, PU_STATIC);
	}

	sbar = W_CachePatchHandle("STBAR", PU_STATIC);
	sbar_width = W_ResolvePatchHandle(sbar)->width();

	int facenum = 0;
	for (int i = 0; i < ST_NUMPAINFACES; i++)
	{
		for (int j = 0; j < ST_NUMSTRAIGHTFACES; j++)
		{
			namebuf = fmt::format("STFST{}{}", i, j);
			faces[facenum++] = LoadFaceGraphic(namebuf);
		}
		namebuf = fmt::format("STFTR{}0", i);
		faces[facenum++] = LoadFaceGraphic(namebuf);
		namebuf = fmt::format("STFTL{}0", i);
		faces[facenum++] = LoadFaceGraphic(namebuf);
		namebuf = fmt::format("STFOUCH{}", i);
		faces[facenum++] = LoadFaceGraphic(namebuf);
		namebuf = fmt::format("STFEVL{}", i);
		faces[facenum++] = LoadFaceGraphic(namebuf);
		namebuf = fmt::format("STFKILL{}", i);
		faces[facenum++] = LoadFaceGraphic(namebuf);
	}
	namebuf = "STFGOD0";
	faces[facenum++] = LoadFaceGraphic(namebuf);
	namebuf = "STFDEAD0";
	faces[facenum] = LoadFaceGraphic(namebuf);
}

static void ST_loadData()
{
	ST_loadGraphics();
}

static void ST_unloadGraphics()
{
	for (int i = 0; i < 10; i++)
	{
		::tallnum[i].clear();
		::shortnum[i].clear();
	}

	::tallpercent.clear();
	::armsbg.clear();
	::flagsbg.clear();

	for (int i = 0; i < 6; i++)
		::arms[i][0].clear();

	for (int i = 0; i < NUMCARDS + NUMCARDS / 2; i++)
		::keys[i].clear();

	::sbar.clear();
	::faceback.clear();

	for (int i = 0; i < ST_NUMFACES; i++)
		::faces[i].clear();

	::negminus.clear();
}

static void ST_unloadData()
{
	ST_unloadGraphics();
	ST_unloadNew();
}

static void ST_createWidgets()
{
	int scaled_x = (sbar_width - 320) / 2;

	w_ready.init(ST_AMMOX + scaled_x, ST_AMMOY, tallnum, &st_current_ammo, ST_AMMOWIDTH);
	w_health.init(ST_HEALTHX + scaled_x, ST_HEALTHY, tallnum, &st_health, tallpercent);

	for (int i = 0; i < 6; i++)
	{
		w_arms[i].init(ST_ARMSX + (i % 3) * ST_ARMSXSPACE + scaled_x,
		               ST_ARMSY + (i / 3) * ST_ARMSYSPACE, arms[i], &st_weaponowned[i]);
	}

	w_frags.init(ST_FRAGSX + scaled_x, ST_FRAGSY, tallnum, &st_fragscount, ST_FRAGSWIDTH);
	w_faces.init(ST_FACESX + scaled_x, ST_FACESY, faces, &st_faceindex);
	w_armor.init(ST_ARMORX + scaled_x, ST_ARMORY, tallnum, &st_armor, tallpercent);

	w_keyboxes[0].init(ST_KEY0X + scaled_x, ST_KEY0Y, keys, &keyboxes[0]);
	w_keyboxes[1].init(ST_KEY1X + scaled_x, ST_KEY1Y, keys, &keyboxes[1]);
	w_keyboxes[2].init(ST_KEY2X + scaled_x, ST_KEY2Y, keys, &keyboxes[2]);

	w_ammo[0].init(ST_AMMO0X + scaled_x, ST_AMMO0Y, shortnum, &st_ammo[0], ST_AMMO0WIDTH);
	w_ammo[1].init(ST_AMMO1X + scaled_x, ST_AMMO1Y, shortnum, &st_ammo[1], ST_AMMO1WIDTH);
	w_ammo[2].init(ST_AMMO2X + scaled_x, ST_AMMO2Y, shortnum, &st_ammo[2], ST_AMMO2WIDTH);
	w_ammo[3].init(ST_AMMO3X + scaled_x, ST_AMMO3Y, shortnum, &st_ammo[3], ST_AMMO3WIDTH);

	w_maxammo[0].init(ST_MAXAMMO0X + scaled_x, ST_MAXAMMO0Y, shortnum, &st_maxammo[0],
	                  ST_MAXAMMO0WIDTH);
	w_maxammo[1].init(ST_MAXAMMO1X + scaled_x, ST_MAXAMMO1Y, shortnum, &st_maxammo[1],
	                  ST_MAXAMMO1WIDTH);
	w_maxammo[2].init(ST_MAXAMMO2X + scaled_x, ST_MAXAMMO2Y, shortnum, &st_maxammo[2],
	                  ST_MAXAMMO2WIDTH);
	w_maxammo[3].init(ST_MAXAMMO3X + scaled_x, ST_MAXAMMO3Y, shortnum, &st_maxammo[3],
	                  ST_MAXAMMO3WIDTH);

	w_lives.init(ST_FX + 34 + scaled_x, ST_FY + 25, shortnum, &st_lives, 2);
}

void ST_DoomTicker()
{
	ST_UpdateSurfaceBpp();
	if (!multiplayer && !demoplayback && (ConsoleState == c_down || ConsoleState == c_falling))
		return;
	st_randomnumber = M_Random();
	ST_updateWidgets();
	st_oldhealth = displayplayer().health;
}

void ST_DoomDrawer()
{
	if (st_needrefresh)
		st_statusbaron = R_StatusBarVisible();

	if (!st_statusbaron)
		return;

	IWindowSurface* surface = R_GetRenderingSurface();
	const int surface_width = surface->getWidth();
	const int surface_height = surface->getHeight();

	ST_WIDTH = ST_StatusBarWidth(surface_width, surface_height);
	ST_HEIGHT = ST_StatusBarHeight(surface_width, surface_height);
	ST_X = ST_StatusBarX(surface_width, surface_height);
	ST_Y = ST_StatusBarY(surface_width, surface_height);

	stbar_surface->lock();
	stnum_surface->lock();

	if (st_needrefresh)
	{
		ST_refreshBackground();

		if (st_scale)
			stnum_surface->blitcrop(stbar_surface, 0, 0, stbar_surface->getWidth(),
			                        stbar_surface->getHeight(), 0, 0, stnum_surface->getWidth(),
			                        stnum_surface->getHeight());
		else
			surface->blitcrop(stbar_surface, 0, 0, stbar_surface->getWidth(),
			                  stbar_surface->getHeight(), ST_X, ST_Y, ST_WIDTH, ST_HEIGHT);
	}

	ST_drawWidgets(st_needrefresh);

	if (st_scale)
		surface->blitcrop(stnum_surface, 0, 0, stnum_surface->getWidth(),
		                  stnum_surface->getHeight(), ST_X, ST_Y, ST_WIDTH, ST_HEIGHT);

	stbar_surface->unlock();
	stnum_surface->unlock();

	st_needrefresh = false;
}

void ST_DoomStart()
{
	ST_ForceRefresh();

	st_gamestate = FirstPersonState;
	st_statusbaron = true;
	st_oldchat = st_chat = false;
	st_cursoron = false;
	st_faceindex = 0;
	st_oldhealth = -1;

	for (int i = 0; i < NUMWEAPONS; i++)
		oldweaponsowned[i] = displayplayer().weaponowned[i];

	for (int i = 0; i < 3; i++)
		keyboxes[i] = -1;

	ST_initNew();
	ST_createWidgets();
}

void ST_DoomInit()
{
	ST_loadData();

	if (stbar_surface == nullptr)
		stbar_surface = I_AllocateSurface(sbar_width, 32, I_GetVideoBitDepth() == 32 ? 32 : 8);

	if (stnum_surface == nullptr)
		stnum_surface = I_AllocateSurface(sbar_width, 32, I_GetVideoBitDepth() == 32 ? 32 : 8);
}

void ST_DoomShutdown()
{
	ST_unloadData();

	I_FreeSurface(stbar_surface);
	stbar_surface = nullptr;
	I_FreeSurface(stnum_surface);
	stnum_surface = nullptr;

	sbar_width = 0;
}

stbarfns_t DoomStatusBar = {
    32,
    ST_DoomResponder,
    ST_DoomTicker,
    ST_DoomDrawer,
    ST_DoomStart,
    ST_DoomInit,
    ST_DoomShutdown,
};

VERSION_CONTROL(st_doom_cpp, "$Id$")
