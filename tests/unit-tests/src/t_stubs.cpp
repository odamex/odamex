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
bool simulated_connection;
gamestate_t gamestate;

CVAR_FUNC_IMPL (sv_allowwidescreen) {}
CVAR_FUNC_IMPL (sv_sharekeys) {}
CVAR_RANGE (sv_teamsinplay, "2", "Teams that are enabled", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 2.0f, 3.0f)
CVAR (sv_maxplayersperteam, "0", "Maximum number of players that can be on a team", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
CVAR (sv_maxplayers,		"0", "maximum players who can join the game, others are spectators", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)

void C_AddTabCommand(char const *) {}
void C_RemoveTabCommand(char const *) {}
void P_ShowSpawns(MapThing*) {}
void G_DeathMatchSpawnPlayer(player_t&) {}
player_t& consoleplayer() { static player_t fake{}; return fake; }
player_t& displayplayer() { static player_t fake{}; return fake; }

void BuildDefaultShademap(palette_t const *, shademap_t &) {}
palindex_t V_BestColor(const argb_t* palette_colors, int r, int g, int b) { return 0; }
palindex_t V_BestColor(const argb_t *palette_colors, argb_t color) { return 0; }
bool R_AlignFlat(int,int,int) { return false; }

void D_SendServerInfoChange(const cvar_t *cvar, const char *value) {}
void D_DoServerInfoChange(byte **stream) {}
void D_WriteUserInfoStrings(int i, byte **stream, bool compact) {}
void D_ReadUserInfoStrings(int i, byte **stream, bool update) {}

void SV_SpawnMobj(AActor *mobj) {}
void SV_TouchSpecial(AActor *special, player_t *player) {}
ItemEquipVal SV_FlagTouch (player_t &player, team_t f, bool firstgrab) { return IEV_NotEquipped; }
void SV_SocketTouch (player_t &player, team_t f) {}
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill) {}
void SV_SendDamagePlayer(player_t *player, AActor* inflictor, int healthDamage, int armorDamage) {}
void SV_SendDamageMobj(AActor *target, int pain) {}
void SV_CTFEvent(team_t f, flag_score_t event, player_t &who) {}
void SV_UpdateFrags(player_t &player) {}
void SV_ActorTarget(AActor *actor) {}
void SV_SendDestroyActor(AActor *mo) {}
void SV_ExplodeMissile(AActor *mo) {}
void SV_SendPlayerInfo(player_t &player) {}
void SV_PreservePlayer(player_t &player) {}
void SV_BroadcastSector(int sectornum) {}
void SV_UpdateMobj(AActor* mo) {}
void SV_UpdateMobjState(AActor* mo) {}

void CTF_RememberFlagPos(mapthing2_t *mthing) {}
void CTF_SpawnFlag(team_t f) {}
bool SV_AwarenessUpdate(player_t &pl, AActor* mo) { return true; }
void SV_SendPackets(void) {}
void SV_SendExecuteLineSpecial(byte special, line_t* line, AActor* activator, int arg0,
                               int arg1, int arg2, int arg3, int arg4) {}

void SV_UpdateMonsterRespawnCount() {}
void SV_Sound(AActor* mo, byte channel, const char* name, byte attenuation) {}

void R_ExitLevel() {}
void D_SetupUserInfo (void) {}
void D_UserInfoChanged (cvar_t *cvar) {}

argb_t V_GetColorFromString(const std::string& str)
{
    return 0;
}

void PickupMessage(AActor *toucher, const char *message) {}
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon) {}

void AM_Stop(void) {}

void RefreshPalettes (void) {}

void V_RefreshColormaps() {}

size_t C_BasePrint(const int printlevel, const char* color_code, const std::string& str) { return 0; }

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug() {}


//
// Internals.
//

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume)
{
	// [RH] Read in sound sequences
	//NumSequences = 0;
}

void S_Start() {}
void S_Stop() {}
void S_SoundID(int channel, int sound_id, float volume, int attenuation) {}
void S_SoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation) {}
void S_SoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation) {}
void S_LoopedSoundID(AActor *ent, int channel, int sound_id, float volume, int attenuation) {}
void S_LoopedSoundID(fixed_t *pt, int channel, int sound_id, float volume, int attenuation) {}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(int channel, const char *name, float volume, int attenuation) {}
void S_Sound(AActor *ent, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_LoopedSound(AActor *ent, int channel, const char *name, float volume, int attenuation) {}
void S_LoopedSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation) {}
void S_Sound(fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation) {}
void S_StopSound(fixed_t *pt) {}
void S_StopSound(fixed_t *pt, int channel) {}
void S_StopSound(AActor *ent, int channel) {}
void S_StopAllChannels() {}

// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound(AActor *from, AActor *to) {}

bool S_GetSoundPlayingInfo(fixed_t *pt, int sound_id)
{
	return false;
}

bool S_GetSoundPlayingInfo(AActor *ent, int sound_id)
{
	return S_GetSoundPlayingInfo (ent ? &ent->x : NULL, sound_id);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound() {}

void S_ResumeSound() {}

//
// Updates music & sounds
//
void S_UpdateSounds(void *listener_p) {}

void S_UpdateMusic() {}

void S_SetMusicVolume(float volume) {}

void S_SetSfxVolume(float volume) {}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(const char *m_id) {}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic(std::string musicname, bool looping)  {}

void S_StopMusic() {}


// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

static struct AmbientSound {
	unsigned	type;		// type of ambient sound
	int			periodmin;	// # of tics between repeats
	int			periodmax;	// max # of tics for random ambients
	float		volume;		// relative volume of sound
	float		attenuation;
	char		sound[MAX_SNDNAME+1]; // Logical name of sound to play
} Ambients[256];

#define RANDOM		1
#define PERIODIC	2
#define CONTINUOUS	3
#define POSITIONAL	4
#define SURROUND	16

void S_HashSounds()
{
	// Mark all buckets as empty
	for (unsigned i = 0; i < S_sfx.size(); i++)
		S_sfx[i].index = ~0;

	// Now set up the chains
	for (unsigned i = 0; i < S_sfx.size(); i++)
	{
		const unsigned j = MakeKey(S_sfx[i].name) % static_cast<unsigned>(S_sfx.size() - 1);
		S_sfx[i].next = S_sfx[j].index;
		S_sfx[j].index = i;
	}
}

int S_FindSound(const char *logicalname)
{
	if (S_sfx.empty())
		return -1;

	int i = S_sfx[MakeKey(logicalname) % static_cast<unsigned>(S_sfx.size() - 1)].index;

	while ((i != -1) && strnicmp(S_sfx[i].name, logicalname, MAX_SNDNAME))
		i = S_sfx[i].next;

	return i;
}

int S_FindSoundByLump(int lump)
{
	if (lump != -1)
	{
		for (unsigned i = 0; i < S_sfx.size(); i++)
			if (S_sfx[i].lumpnum == lump)
				return i;
	}
	return -1;
}

int S_AddSoundLump(const char *logicalname, int lump)
{
	S_sfx.push_back(sfxinfo_t());
	sfxinfo_t& new_sfx = S_sfx[S_sfx.size() - 1];

	// logicalname MUST be < MAX_SNDNAME chars long
	strcpy(new_sfx.name, logicalname);
	new_sfx.data = NULL;
	new_sfx.link = sfxinfo_t::NO_LINK;
	new_sfx.lumpnum = lump;
	return S_sfx.size() - 1;
}

void S_ClearSoundLumps()
{
	S_sfx.clear();
	S_rnd.clear();
}

int FindSoundNoHash(const char* logicalname)
{
	for (size_t i = 0; i < S_sfx.size(); i++)
		if (iequals(logicalname, S_sfx[i].name))
			return i;

	return S_sfx.size();
}

int FindSoundTentative(const char* name)
{
	int id = FindSoundNoHash(name);
	if (id == static_cast<int>(S_sfx.size()))
	{
		id = S_AddSoundLump(name, -1);
	}
	return id;
}

int S_AddSound(const char *logicalname, const char *lumpname)
{
	int sfxid = FindSoundNoHash(logicalname);

	const int lump = lumpname ? W_CheckNumForName(lumpname) : -1;

	// Otherwise, prepare a new one.
	if (sfxid != static_cast<int>(S_sfx.size()))
	{
		sfxinfo_t& sfx = S_sfx[sfxid];

		sfx.lumpnum = lump;
		sfx.link = sfxinfo_t::NO_LINK;
		if (sfx.israndom)
		{
			S_rnd.erase(sfxid);
			sfx.israndom = false;
		}
	}
	else
		sfxid = S_AddSoundLump(logicalname, lump);

	return sfxid;
}

void S_AddRandomSound(int owner, std::vector<int>& list)
{
	S_rnd[owner] = list;
	S_sfx[owner].link = owner;
	S_sfx[owner].israndom = true;
}

// S_ParseSndInfo
// Parses all loaded SNDINFO lumps.
void S_ParseSndInfo()
{
	S_ClearSoundLumps();

	int lump = -1;
	while ((lump = W_FindLump("SNDINFO", lump)) != -1)
	{
		char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_CACHE));

		const OScannerConfig config = {
		    "SNDINFO", // lumpName
		    true,      // semiComments
		    true,      // cComments
		};
		OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

		while (os.scan())
		{
			std::string tok = os.getToken();

			// check if token is a command
			if (tok[0] == '$')
			{
				os.mustScan();
				if (os.compareTokenNoCase("ambient"))
				{
					// $ambient <num> <logical name> [point [atten]|surround] <type>
					// [secs] <relative volume>
					AmbientSound *ambient, dummy;

					os.mustScanInt();
					const int index = os.getTokenInt();
					if (index < 0 || index > 255)
					{
						os.warning("Bad ambient index (%d)\n", index);
						ambient = &dummy;
					}
					else
					{
						ambient = Ambients + index;
					}

					ambient->type = 0;
					ambient->periodmin = 0;
					ambient->periodmax = 0;
					ambient->volume = 0.0f;

					os.mustScan();
					strncpy(ambient->sound, os.getToken().c_str(), MAX_SNDNAME);
					ambient->sound[MAX_SNDNAME] = 0;
					ambient->attenuation = 0.0f;

					os.mustScan();
					if (os.compareTokenNoCase("point"))
					{
						ambient->type = POSITIONAL;
						os.mustScan();

						if (IsRealNum(os.getToken().c_str()))
						{
							ambient->attenuation =
							    (os.getTokenFloat() > 0) ? os.getTokenFloat() : 1;
							os.mustScan();
						}
						else
						{
							ambient->attenuation = 1;
						}
					}
					else if (os.compareTokenNoCase("surround"))
					{
						ambient->type = SURROUND;
						os.mustScan();
						ambient->attenuation = -1;
					}
					// else if (os.compareTokenNoCase("world"))
					//{
					// todo
					//}

					if (os.compareTokenNoCase("continuous"))
					{
						ambient->type |= CONTINUOUS;
					}
					else if (os.compareTokenNoCase("random"))
					{
						ambient->type |= RANDOM;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
						os.mustScanFloat();
						ambient->periodmax =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else if (os.compareTokenNoCase("periodic"))
					{
						ambient->type |= PERIODIC;
						os.mustScanFloat();
						ambient->periodmin =
						    static_cast<int>(os.getTokenFloat() * TICRATE);
					}
					else
					{
						os.warning("Unknown ambient type (%s)\n", os.getToken().c_str());
					}

					os.mustScanFloat();
					ambient->volume = clamp(os.getTokenFloat(), 0.0f, 1.0f);
				}
				else if (os.compareTokenNoCase("map"))
				{
					// Hexen-style $MAP command
					os.mustScanInt();
					OLumpName mapname = fmt::format("MAP{:02d}", os.getTokenInt());
					level_pwad_info_t& info = getLevelInfos().findByName(mapname);
					os.mustScan();
					if (info.mapname[0])
					{
						info.music = os.getToken();
					}
				}
				else if (os.compareTokenNoCase("alias"))
				{
					os.mustScan();
					const int sfxfrom = S_AddSound(os.getToken().c_str(), NULL);
					os.mustScan();
					S_sfx[sfxfrom].link = FindSoundTentative(os.getToken().c_str());
				}
				else if (os.compareTokenNoCase("random"))
				{
					std::vector<int> list;

					os.mustScan();
					const int owner = S_AddSound(os.getToken().c_str(), NULL);

					os.mustScan();
					os.assertTokenIs("{");
					while (os.scan() && !os.compareToken("}"))
					{
						const int sfxto = FindSoundTentative(os.getToken().c_str());

						if (owner == sfxto)
						{
							os.warning("Definition of random sound '%s' refers to itself "
							           "recursively.\n",
							           os.getToken().c_str());
							continue;
						}

						list.push_back(sfxto);
					}
					if (list.size() == 1)
					{
						// only one sound; treat as alias
						S_sfx[owner].link = list[0];
					}
					else if (list.size() > 1)
					{
						S_AddRandomSound(owner, list);
					}
				}
				else
				{
					os.warning("Unknown SNDINFO command %s\n", os.getToken().c_str());
					while (os.scan())
						if (os.crossed())
						{
							os.unScan();
							break;
						}
				}
			}
			else
			{
				// token is a logical sound mapping
				char name[MAX_SNDNAME + 1];

				strncpy(name, tok.c_str(), MAX_SNDNAME);
				name[MAX_SNDNAME] = 0;
				os.mustScan();
				S_AddSound(name, os.getToken().c_str());
			}
		}
	}
	S_HashSounds();
}

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
void UV_SoundAvoidPlayer(AActor *mo, byte channel, const char *name, byte attenuation) {}
void OnChangedSwitchTexture (line_t *line, int useAgain) {}
void C_MidPrint (const char *msg, player_t *p, int msgtime) {}

#define R_P2ATHRESHOLD (INT_MAX / 4)

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
translationref_t::translationref_t(const translationref_t &other) : m_table(other.m_table), m_player_id(other.m_player_id) {}
translationref_t::translationref_t(const byte *table) : m_table(table), m_player_id(-1) {}
translationref_t::translationref_t(const byte *table, const int player_id) : m_table(table), m_player_id(player_id) {}

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

static palette_t default_palette;

const palette_t* V_GetDefaultPalette()
{
	return &default_palette;
}

void P_SpawnPlayer(player_t&, mapthing2_t*) {}
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

shaderef_t::shaderef_t() : m_colors(NULL), m_mapnum(-1), m_colormap(NULL), m_shademap(NULL) {}

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

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
	int index = ang >> ANGLETOFINESHIFT;

	tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
	ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

VERSION_CONTROL (test_stubs_cpp, "$Id$")
