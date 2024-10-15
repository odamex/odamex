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
//	[RH] p_acs.c: New file to handle ACS scripts
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "z_zone.h"
#include "p_local.h"
#include "p_spec.h"
#include "s_sound.h"
#include "p_acs.h"
#include "p_saveg.h"
#include "p_lnspec.h"
#include "m_random.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "s_sndseq.h"
#include "i_system.h"
#include "m_vectors.h"
#include "p_inter.h"
#include "gi.h"

#if defined(SERVER_APP)
#include "sv_main.h"
#endif

#define CLAMPCOLOR(c)	(EColorRange)((unsigned)(c)>CR_UNTRANSLATED?CR_UNTRANSLATED:(c))
#define LANGREGIONMASK	MAKE_ID(0,0,0xff,0xff)

struct CallReturn
{
	int ReturnAddress;
	ScriptFunction *ReturnFunction;
	BYTE bDiscardResult;
	BYTE Pad[3];
};

static int Stack[STACK_SIZE];

static bool P_GetScriptGoing (AActor *who, line_t *where, int num, int *code,
	int lineSide, int arg0, int arg1, int arg2, int always, bool delay);
AActor* P_FindThingById(uint32_t id);

template <size_t N>
static std::vector<int> ArgvToArgs(const int (&a)[N])
{
	return std::vector<int>(a, a + N);
}

struct FBehavior::ArrayInfo
{
	int ArraySize;
	SDWORD *Elements;
};

static void DoClearInv(player_t* player)
{
	memset(player->weaponowned, 0, sizeof(player->weaponowned));
	memset(player->powers, 0, sizeof(player->powers));
	memset(player->cards, 0, sizeof(player->cards));
	memset(player->ammo, 0, sizeof(player->ammo));

	if (player->backpack)
	{
		player->backpack = false;
		for (int i = 0; i < NUMAMMO; i++)
		{
			player->maxammo[i] /= 2;
		}
	}

	player->pendingweapon = NUMWEAPONS;

	SERVER_ONLY(
	    SV_ACSExecuteSpecial(DLevelScript::PCD_CLEARINVENTORY, player->mo, NULL, true));
}

static void ClearInventory(AActor* activator)
{
	if (activator == NULL)
	{
		for (Players::iterator it = players.begin(); it != players.end(); ++it)
		{
			if (it->ingame() && !it->spectator)
			{
				DoClearInv(&(*it));
				SERVER_ONLY(SV_SendPlayerInfo(*it));
			}
		}
	}
	else if (activator->player != NULL)
	{
		DoClearInv(activator->player);
		SERVER_ONLY(SV_SendPlayerInfo(*(activator->player)));
	}
}

// TODO: HACKY WACKY, need to port classes from ZDOOM 1.23 to make that way cleaner
struct DoomEntity{
	const char* Name;
	mobjtype_t Type;
};
#define NUMMONSTERS 22

static DoomEntity DoomMonsterNames[NUMMONSTERS] = {
    {"ZombieMan", MT_POSSESSED},  {"ShotgunGuy", MT_SHOTGUY},
    {"ChaingunGuy", MT_CHAINGUY}, {"DoomImp", MT_TROOP},
    {"Demon", MT_SERGEANT},       {"Spectre", MT_SHADOWS},
    {"LostSoul", MT_SKULL},       {"Cacodemon", MT_HEAD},
    {"HellKnight", MT_KNIGHT},    {"BaronOfHell", MT_BRUISER},
    {"Arachnotron", MT_BABY},     {"PainElemental", MT_PAIN},
    {"Revenant", MT_UNDEAD},      {"Fatso", MT_FATSO},
    {"Archvile", MT_VILE},        {"SpiderMastermind", MT_SPIDER},
    {"Cyberdemon", MT_CYBORG},    {"CommanderKeen", MT_KEEN},
    {"WolfensteinSS", MT_WOLFSS}, {"BossBrain", MT_BOSSBRAIN},
    {"BossTarget", MT_BOSSTARGET}, {"BossEye", MT_BOSSSPIT}};

static DoomEntity DoomAmmoNames[8] = {{"Clip", MT_CLIP},        {"Shell", MT_MISC22},
                                      {"Cell", MT_MISC20},      {"RocketAmmo", MT_MISC18},
                                      {"ClipBox", MT_MISC17},   {"ShellBox", MT_MISC23},
                                      {"RocketBox", MT_MISC19}, {"CellPack", MT_MISC21}};

static DoomEntity DoomWeaponNames[9] = {
    {"Fist", MT_NULL}, // Default weapon, no entity
    {"Pistol", MT_NULL},          {"Shotgun", MT_SHOTGUN},
    {"Chaingun", MT_CHAINGUN},    {"RocketLauncher", MT_MISC27},
    {"PlasmaRifle", MT_MISC28},   {"BFG9000", MT_MISC25},
    {"Chainsaw", MT_MISC26},      {"SuperShotgun", MT_SUPERSHOTGUN}};

static DoomEntity DoomKeyNames[6] = {{"BlueCard", MT_MISC4},
                                     {"YellowCard", MT_MISC6},
                                     {"RedCard", MT_MISC5},
                                     {"BlueSkull", MT_MISC9},
                                     {"YellowSkull", MT_MISC7},
									 {"RedSkull", MT_MISC8}};

static DoomEntity DoomPowerNames[7] = {{"InvulnerabilitySphere", MT_INV},
                                       {"Berserk", MT_MISC13},
                                       {"BlurSphere", MT_INS},
                                       {"RadSuit", MT_MISC14},
                                       {"Allmap", MT_MISC15},
                                       {"Infrared", MT_MISC16}};

static DoomEntity DoomHealthArmorNames[9] = {
    {"HealthBonus", MT_MISC2}, {"ArmorBonus", MT_MISC3}, {"GreenArmor", MT_MISC0},
    {"BlueArmor", MT_MISC1},   {"Stimpack", MT_MISC10},  {"Medikit", MT_MISC11},
    {"Soulsphere", MT_MISC12}, {"Megasphere", MT_MEGA},  {"Backpack", MT_MISC24}};

static DoomEntity DoomDecorationNames[60] = {{"BurningBarrel", MT_MISC77},
                                             {"HangNoGuts", MT_MISC78},
                                             {"HangBNoBrain", MT_MISC79},
                                             {"HangTLookingDown", MT_MISC80},
                                             {"HangTSkull", MT_MISC81},
                                             {"HangTLookingUp", MT_MISC82},
                                             {"HangTNoBrain", MT_MISC83},
                                             {"ColonGibs", MT_MISC84},
                                             {"SmallBloodPool", MT_MISC85},
                                             {"BrainStem", MT_MISC86},
                                             {"TechLamp", MT_MISC29},
                                             {"TechLamp2", MT_MISC30},
                                             {"GibbedMarine", MT_MISC68},
                                             {"GibbedMarineExtra", MT_MISC69},
                                             {"DeadMarine", MT_MISC62},
                                             {"DeadZombieMan", MT_MISC63},
                                             {"DeadShotgunGuy", MT_MISC67},
                                             {"DeadDoomImp", MT_MISC66},
                                             {"DeadDemon", MT_MISC64},
                                             {"DeadCacodemon", MT_MISC61},
                                             {"DeadLostSoul", MT_MISC65},
                                             {"Gibs", MT_MISC71},
                                             {"DeadStick", MT_MISC74},
                                             {"LiveStick", MT_MISC75},
                                             {"HeadOnAStick", MT_MISC72},
                                             {"HeadsOnAStick", MT_MISC70},
                                             {"HeadCandles", MT_MISC73},
                                             {"TallGreenColumn", MT_MISC32},
                                             {"ShortGreenColumn", MT_MISC33},
                                             {"TallRedColumn", MT_MISC34},
                                             {"ShortRedColumn", MT_MISC35},
                                             {"Candlestick", MT_MISC49},
                                             {"Candelabra", MT_MISC50},
                                             {"HeartColumn", MT_MISC37},
                                             {"SkullColumn", MT_MISC36},
                                             {"EvilEye", MT_MISC38},
                                             {"FloatingSkull", MT_MISC39},
                                             {"TorchTree", MT_MISC40},
                                             {"BlueTorch", MT_MISC41},
                                             {"GreenTorch", MT_MISC42},
                                             {"RedTorch", MT_MISC43},
                                             {"Stalagtite", MT_MISC47},
                                             {"TechPillar", MT_MISC48},
                                             {"BloodyTwitch", MT_MISC51},
                                             {"Meat2", MT_MISC52},
                                             {"Meat3", MT_MISC53},
                                             {"Meat4", MT_MISC54},
                                             {"Meat5", MT_MISC55},
                                             {"BigTree", MT_MISC76},
                                             {"ShortBlueTorch", MT_MISC44},
                                             {"ShortGreenTorch", MT_MISC45},
                                             {"ShortRedTorch", MT_MISC46},
                                             {"NonsolidMeat2", MT_MISC56},
                                             {"NonsolidMeat4", MT_MISC57},
                                             {"NonsolidMeat3", MT_MISC58},
                                             {"NonsolidMeat5", MT_MISC59},
                                             {"NonsolidTwitch", MT_MISC60},
                                             {"ZBridge", MT_BRIDGE},
                                             {"Column", MT_MISC31},
                                             {"ExplosiveBarrel", MT_BARREL}};

extern ItemEquipVal P_GiveAmmo(player_t *player, ammotype_t ammo, float num);
extern ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped);
extern ItemEquipVal P_GiveCard(player_t *player, card_t card);
extern ItemEquipVal P_GivePower(player_t *player, int  power);

mobjtype_t FindWeaponEntity(const char* type)
{
	int i;
	for (i = 0; i < 9; i++)
	{
		if (strcmp(DoomWeaponNames[i].Name, type) == 0)
		{
			if (DoomWeaponNames[i].Type == MT_NULL)
			{
				DPrintf("ACS: LOGIC ERROR - Cannot spawn default weapons!\n");
				return MT_NULL;
			}
			return DoomWeaponNames[i].Type;
		}
	}

	return MT_NULL;
}

mobjtype_t FindDoomEntity(const char* type, DoomEntity list[], int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		if (strcmp(list[i].Name, type) == 0)
		{
			return list[i].Type;
		}
	}

	return MT_NULL;
}

static void GiveBackpack(player_t* player)
{
	if (!player->backpack)
	{
		for (int i=0 ; i<NUMAMMO ; i++)
		{
			player->maxammo[i] *= 2;
		}
		player->backpack = true;
	}
	for (int i=0 ; i<NUMAMMO ; i++)
	{
		P_GiveAmmo(player, static_cast<ammotype_t>(i), 1);
	}
	SERVER_ONLY(SV_SendPlayerInfo(*player));
}

static void DoGiveInv(player_t* player, const char* type, int amount)
{
	weapontype_t savedpendingweap = player->pendingweapon;

	// Give ammo
	for (int i = 0; i < NUMAMMO; i++)
	{
		if (strcmp(DoomAmmoNames[i].Name, type) == 0)
		{
			player->ammo[i] = MIN(player->ammo[i]+amount, player->maxammo[i]);
			SERVER_ONLY(SV_SendPlayerInfo(*player));
			return;
		}
	}

	// Give weapon
	for (int i = 0; i < NUMWEAPONS; i++)
	{
		if (strcmp(DoomWeaponNames[i].Name, type) == 0)
		{
			do
			{
				P_GiveWeapon(player, static_cast<weapontype_t>(i), false);
			}
			while (--amount > 0);

			// Don't bring it up automatically
			if (player->readyweapon != NUMWEAPONS && player->pendingweapon != NUMWEAPONS)
				player->pendingweapon = savedpendingweap;
			SERVER_ONLY(SV_SendPlayerInfo(*player));
			return;
		}
	}

	// Give keycard
	for (int i = 0; i < NUMCARDS; i++)
	{
		if (strcmp(DoomKeyNames[i].Name, type) == 0)
		{
			do
			{
				P_GiveCard(player, static_cast<card_t>(i));
			}
			while (--amount > 0);
			SERVER_ONLY(SV_SendPlayerInfo(*player));
			return;
		}
	}

	// Give power
	for (int i = 0; i < NUMPOWERS; i++)
	{
		if (strcmp(DoomPowerNames[i].Name, type) == 0)
		{
			do
			{
				P_GivePower(player, i);
			}
			while (--amount > 0);
			SERVER_ONLY(SV_SendPlayerInfo(*player));
			return;
		}
	}

	// Give backpack
	if (strcmp("Backpack", type) == 0)
	{
		do
		{
			GiveBackpack(player);
		}
		while (--amount > 0);
		SERVER_ONLY(SV_SendPlayerInfo(*player));
		return;
	}

	// Unknown item.
	Printf(PRINT_HIGH, "I don't know what %s is\n", type);
}

static void GiveInventory(AActor* activator, const char* type, int amount)
{
	if (activator == NULL)
	{
		for (int i = 0; i < MAXPLAYERS; ++i)
		{
			Players::iterator it;
			for (it = players.begin();it != players.end();++it)
			{
				if (it->ingame() && !it->spectator)
					DoGiveInv(&(*it), type, amount);
			}
		}
	}
	else if (activator->player != NULL)
	{
		DoGiveInv(activator->player, type, amount);
	}
}

extern void P_SwitchWeapon(player_t *player);

static void TakeWeapon(player_t* player, int weapon)
{
	player->weaponowned[weapon] = false;
	if (player->readyweapon == weapon || player->pendingweapon == weapon)
	{
		P_SwitchWeapon(player);

		bool hasWeapon = false;
		for (int i = 0; i < NUMWEAPONS; i++)
		{
			if (player->weaponowned[i])
			{
				hasWeapon = true;
				break;
			}
		}

		if (!hasWeapon)
			player->pendingweapon = NUMWEAPONS;
	}
	SERVER_ONLY(SV_SendPlayerInfo(*player));
}

extern BOOL P_CheckAmmo (player_t *player);

static void TakeAmmo(player_t* player, int ammo, int amount)
{
	if (amount == 0)
	{
		player->ammo[ammo] = 0;
	}
	else
	{
		player->ammo[ammo] = MAX(player->ammo[ammo]-amount, 0);
	}
	if (player->pendingweapon != wp_nochange)
	{
		// Make sure we have the ammo for the weapon being switched to
		weapontype_t readynow = player->readyweapon;
		player->readyweapon = player->pendingweapon;
		player->pendingweapon = wp_nochange;
		if (P_CheckAmmo(player))
		{
			// There was enough ammo for the pending weapon, so keep switching
			player->pendingweapon = player->readyweapon;
			player->readyweapon = readynow;
		}
		else
		{
			player->pendingweapon = player->readyweapon = readynow;
			P_CheckAmmo(player);
		}
	}
	else
	{
		// Make sure we still have enough ammo for the current weapon
		P_CheckAmmo(player);
	}
	SERVER_ONLY(SV_SendPlayerInfo(*player));
}

static AActor* SingleActorFromTID(int tid, AActor* defactor)
{
	if (tid == 0)
	{
		return defactor;
	}
	else
	{
		FActorIterator iterator(tid);
		return iterator.Next();
	}
}

static void TakeBackpack(player_t* player)
{
	if (!player->backpack)
		return;

	player->backpack = false;
	for (int i = 0; i < NUMAMMO; ++i)
	{
		player->maxammo[i] /= 2;
		if (player->ammo[i] > player->maxammo[i])
		{
			player->ammo[i] = player->maxammo[i];
		}
	}
	SERVER_ONLY(SV_SendPlayerInfo(*player));
}

static void DoTakeInv(player_t* player, const char* type, int amount)
{
	int i;

	for (i = 0; i < NUMAMMO; ++i)
	{
		if (strcmp(DoomAmmoNames[i].Name, type) == 0)
		{
			TakeAmmo(player, i, amount);
			return;
		}
	}
	for (i = 0; i < NUMWEAPONS; ++i)
	{
		if (strcmp(DoomWeaponNames[i].Name, type) == 0)
		{
			TakeWeapon(player, i);
			return;
		}
	}
	for (i = 0; i < NUMCARDS; ++i)
	{
		if (strcmp(DoomKeyNames[i].Name, type) == 0)
		{
			player->cards[i] = 0;
		}
	}
	if (strcmp("Backpack", type) == 0)
	{
		TakeBackpack(player);
	}
}

static void TakeInventory(AActor* activator, const char* type, int amount)
{
	if (activator == NULL)
	{
		Players::iterator it;
		for (it = players.begin();it != players.end();++it)
		{
			if (it->ingame() && !it->spectator)
				DoTakeInv(&(*it), type, amount);
		}
	}
	else if (activator->player != NULL)
	{
		DoTakeInv(activator->player, type, amount);
	}
}

static int CheckInventory(AActor* activator, const char* type)
{
	if (activator == NULL || activator->player == NULL)
		return 0;

	player_t* player = activator->player;

	for (int i = 0; i < NUMAMMO; ++i)
	{
		if (strcmp(DoomAmmoNames[i].Name, type) == 0)
		{
			return player->ammo[i];
		}
	}
	for (int i = 0; i < NUMWEAPONS; ++i)
	{
		if (strcmp(DoomWeaponNames[i].Name, type) == 0)
		{
			return player->weaponowned[i] ? 1 : 0;
		}
	}
	for (int i = 0; i < NUMCARDS; ++i)
	{
		if (strcmp(DoomKeyNames[i].Name, type) == 0)
		{
			return player->cards[i] ? 1 : 0;
		}
	}
	if (strcmp("Backpack", type) == 0)
	{
		return player->backpack ? 1 : 0;
	}
	return 0;
}

EXTERN_CVAR (sv_skill)
EXTERN_CVAR (sv_gametype)

//---- ACS lump manager ----//

FBehavior::FBehavior (BYTE *object, int len)
{
	int i;

	NumScripts = 0;
	NumFunctions = 0;
	NumArrays = 0;
	Scripts = NULL;
	Functions = NULL;
	Arrays = NULL;
	Chunks = NULL;

	if (object[0] != 'A' || object[1] != 'C' || object[2] != 'S')
	{
		Format = ACS_Unknown;
		return;
	}

	switch (object[3])
	{
	case 0:
		Format = ACS_Old;
		break;
	case 'E':
		Format = ACS_Enhanced;
		break;
	case 'e':
		Format = ACS_LittleEnhanced;
		break;
	default:
		Format = ACS_Unknown;
		return;
	}

	Data = object;
	DataSize = len;

	if (Format == ACS_Old)
	{
		Chunks = object + len;
		Scripts = object + ((DWORD *)object)[1];
		NumScripts = ((DWORD *)Scripts)[0];
		// Check for redesigned ACSE/ACSe
		if (((DWORD *)object)[1] >= 6*4 &&
			(((DWORD *)Scripts)[-1] == MAKE_ID('A','C','S','e') ||
			((DWORD *)Scripts)[-1] == MAKE_ID('A','C','S','E')))
		{
			Format = (((BYTE *)Scripts)[-1] == 'e') ? ACS_LittleEnhanced : ACS_Enhanced;
			Chunks = object + ((DWORD *)Scripts)[-2];
			// Forget about the compatibility cruft at the end of the lump
			DataSize = ((DWORD *)object)[1] - 8;
		}
		else
		{
			Scripts += 4;
			for (i = 0; i < NumScripts; ++i)
			{
				ScriptPtr2 ptr1 = *(ScriptPtr2 *)(Scripts + 12*i);
				ScriptPtr *ptr2 =  (ScriptPtr  *)(Scripts +  8*i);
				ptr2->Number = ptr1.Number % 1000;
				ptr2->Type = ptr1.Number / 1000;
				ptr2->ArgCount = ptr1.ArgCount;
				ptr2->Address = ptr1.Address;
			}
		}
	}
	else
	{
		Chunks = object + ((DWORD *)object)[1];
	}
	if (Format != ACS_Old)
	{
		Scripts = FindChunk (MAKE_ID('S','P','T','R'));
		if (object[3] != 0)
		{
			NumScripts = ((DWORD *)Scripts)[1] / 12;
			Scripts += 8;
			for (i = 0; i < NumScripts; ++i)
			{
				ScriptPtr1 ptr1 = *(ScriptPtr1 *)(Scripts + 12*i);
				ScriptPtr *ptr2 =  (ScriptPtr  *)(Scripts +  8*i);
				ptr2->Number = ptr1.Number;
				ptr2->Type = ptr1.Type;
				ptr2->ArgCount = ptr1.ArgCount;
				ptr2->Address = ptr1.Address;
			}
		}
		else
		{
			NumScripts = ((DWORD *)Scripts)[1] / 8;
			Scripts += 8;
		}
	}

	// Sort scripts, so we can use a binary search to find them
	if (NumScripts > 0)
	{
		qsort (Scripts, NumScripts, 8, SortScripts);
	}

	if (Format == ACS_Old)
	{
		LanguageNeutral = ((DWORD *)Data)[1];
		LanguageNeutral += ((DWORD *)(Data + LanguageNeutral))[0] * 12 + 4;
	}
	else
	{
		LanguageNeutral = FindLanguage (0, false);
		PrepLocale (LanguageIDs[0], LanguageIDs[1], LanguageIDs[2], LanguageIDs[3]);
	}

	if (Format != ACS_Old)
	{
		DWORD *chunk;

		Functions = FindChunk(MAKE_ID('F','U','N','C'));
		if (Functions != NULL)
		{
			NumFunctions = LELONG(((DWORD *)Functions)[1]);
			Functions += 8;
		}

		chunk = (DWORD *)FindChunk(MAKE_ID('M','I','N','I'));
		if (chunk != NULL)
		{
			int numvars = LELONG(chunk[1])/4;
			int firstvar = LELONG(chunk[2]);
			for (i = 0; i < numvars; ++i)
			{
				level.vars[i+firstvar] = LELONG(chunk[3+i]);
			}
		}

		chunk = (DWORD *)FindChunk(MAKE_ID('A','R','A','Y'));
		if (chunk != NULL)
		{
			NumArrays = LELONG(chunk[1])/8;
			Arrays = new ArrayInfo[NumArrays];
			memset (Arrays, 0, sizeof(*Arrays)*NumArrays);
			for (i = 0; i < NumArrays; ++i)
			{
				level.vars[LELONG(chunk[2+i*2])] = i;
				Arrays[i].ArraySize = LELONG(chunk[3+i*2]);
				Arrays[i].Elements = new SDWORD[Arrays[i].ArraySize];
				memset(Arrays[i].Elements, 0, Arrays[i].ArraySize*sizeof(DWORD));
			}
		}

		chunk = (DWORD *)FindChunk(MAKE_ID('A','I','N','I'));
		while (chunk != NULL)
		{
			int arraynum = level.vars[LELONG(chunk[2])];
			if ((unsigned)arraynum < (unsigned)NumArrays)
			{
				int initsize = MIN<int> (Arrays[arraynum].ArraySize, (LELONG(chunk[1])-4)/4);
				SDWORD *elems = Arrays[arraynum].Elements;
				for (i = 0; i < initsize; ++i)
				{
					elems[i] = LELONG(chunk[3+i]);
				}
			}
			chunk = (DWORD *)NextChunk((BYTE *)chunk);
		}
	}

	DPrintf ("Loaded %d scripts, %d Functions\n", NumScripts, NumFunctions);
}

FBehavior::~FBehavior ()
{
	// Object file is freed by the zone heap
	if(Arrays != NULL)
	{
		for (int i = 0; i < NumArrays; ++i)
		{
			if (Arrays[i].Elements != NULL)
			{
				delete[] Arrays[i].Elements;
				Arrays[i].Elements = NULL;
			}
		}
		delete[] Arrays;
		Arrays = NULL;
	}
}

int STACK_ARGS FBehavior::SortScripts (const void *a, const void *b)
{
	ScriptPtr *ptr1 = (ScriptPtr *)a;
	ScriptPtr *ptr2 = (ScriptPtr *)b;
	return ptr1->Number - ptr2->Number;
}

bool FBehavior::IsGood ()
{
	return Format != ACS_Unknown;
}

int *FBehavior::FindScript (int script) const
{
	const ScriptPtr *ptr = BinarySearch<ScriptPtr, WORD>
		((ScriptPtr *)Scripts, NumScripts, &ScriptPtr::Number, (WORD)script);

	return ptr ? (int *)(ptr->Address + Data) : NULL;
}

ScriptFunction *FBehavior::GetFunction (int funcnum) const
{
	if ((unsigned)funcnum >= (unsigned)NumFunctions)
	{
		return NULL;
	}
	return (ScriptFunction *)Functions + funcnum;
}

int FBehavior::GetArrayVal (int arraynum, int index) const
{
	if ((unsigned)arraynum >= (unsigned)NumArrays)
		return 0;
	const ArrayInfo *array = &Arrays[arraynum];
	if ((unsigned)index >= (unsigned)array->ArraySize)
		return 0;
	return array->Elements[index];
}

void FBehavior::SetArrayVal (int arraynum, int index, int value)
{
	if ((unsigned)arraynum >= (unsigned)NumArrays)
		return;
	const ArrayInfo *array = &Arrays[arraynum];
	if ((unsigned)index >= (unsigned)array->ArraySize)
		return;
	array->Elements[index] = value;
}

BYTE *FBehavior::FindChunk (DWORD id) const
{
	BYTE *chunk = Chunks;

	while (chunk != NULL && chunk < Data + DataSize)
	{
		if (((DWORD *)chunk)[0] == id)
		{
			return chunk;
		}
		chunk += ((DWORD *)chunk)[1] + 8;
	}
	return NULL;
}

BYTE *FBehavior::NextChunk (BYTE *chunk) const
{
	DWORD id = *(DWORD *)chunk;
	chunk += ((DWORD *)chunk)[1] + 8;
	while (chunk != NULL && chunk < Data + DataSize)
	{
		if (((DWORD *)chunk)[0] == id)
		{
			return chunk;
		}
		chunk += ((DWORD *)chunk)[1] + 8;
	}
	return NULL;
}

const char *FBehavior::LookupString (DWORD index, DWORD ofs) const
{
	if (Format == ACS_Old)
	{
		DWORD *list = (DWORD *)(Data + LanguageNeutral);

		if (index >= list[0])
			return NULL;	// Out of range for this list;
		return (const char *)(Data + list[1+index]);
	}
	else
	{
		if (ofs == 0)
		{
			ofs = LanguageNeutral;
			if (ofs == 0)
			{
				return NULL;
			}
		}
		DWORD *list = (DWORD *)(Data + ofs);

		if (index >= list[1])
			return NULL;	// Out of range for this list
		if (list[3+index] == 0)
			return NULL;	// Not defined in this list
		return (const char *)(Data + ofs + list[3+index]);
	}
}

const char *FBehavior::LocalizeString (DWORD index) const
{
	if (Format != ACS_Old)
	{
		DWORD ofs = Localized;
		const char *str = NULL;

		while (ofs != 0 && (str = LookupString (index, ofs)) == NULL)
		{
			ofs = ((DWORD *)(Data + ofs))[2];
		}
		return str;
	}
	else
	{
		return LookupString (index);
	}
}

void FBehavior::PrepLocale (DWORD userpref, DWORD userdef, DWORD syspref, DWORD sysdef)
{
	BYTE *chunk;
	DWORD *list;

	// Clear away any existing links
	for (chunk = Chunks; chunk < Data + DataSize; chunk += ((DWORD *)chunk)[1] + 8)
	{
		list = (DWORD *)chunk;
		if (list[0] == MAKE_ID('S','T','R','L'))
		{
			list[4] = 0;
		}
	}
	Localized = 0;

	if (userpref)
		AddLanguage (userpref);
	if (userpref & LANGREGIONMASK)
		AddLanguage (userpref & ~LANGREGIONMASK);
	if (userdef)
		AddLanguage (userdef);
	if (userdef & LANGREGIONMASK)
		AddLanguage (userdef & ~LANGREGIONMASK);
	if (syspref)
		AddLanguage (syspref);
	if (syspref & LANGREGIONMASK)
		AddLanguage (syspref & ~LANGREGIONMASK);
	if (sysdef)
		AddLanguage (sysdef);
	if (sysdef & LANGREGIONMASK)
		AddLanguage (sysdef & ~LANGREGIONMASK);
	AddLanguage (MAKE_ID('e','n',0,0));		// Use English as a fallback
	AddLanguage (0);			// Failing that, use language independent strings
}

void FBehavior::AddLanguage (DWORD langid)
{
	DWORD ofs, *ofsput;
	DWORD *list;
	BYTE *chunk;

	// First, make sure language is not already inserted
	ofsput = CheckIfInList (langid);
	if (ofsput == NULL)
	{ // Already in list
		return;
	}

	// Try to find an exact match first
	ofs = FindLanguage (langid, false);
	if (ofs != 0)
	{
		*ofsput = ofs;
		return;
	}

	// If langid has no sublanguage, add all languages that match the major
	// type, if not in list already
	if ((langid & LANGREGIONMASK) == 0)
	{
		for (chunk = Chunks; chunk < Data + DataSize; chunk += ((DWORD *)chunk)[1] + 8)
		{
			list = (DWORD *)chunk;
			if (list[0] != MAKE_ID('S','T','R','L'))
				continue;	// not a string list
			if ((list[2] & ~LANGREGIONMASK) != langid)
				continue;	// wrong language
			if (list[4] != 0)
				continue;	// definitely in language list
			ofsput = CheckIfInList (list[2]);
			if (ofsput != NULL)
				*ofsput = chunk - Data + 8;	// add to language list
		}
	}
}

DWORD *FBehavior::CheckIfInList (DWORD langid)
{
	DWORD ofs, *ofsput;
	DWORD *list;

	ofs = Localized;
	ofsput = &Localized;
	while (ofs != 0)
	{
		list = (DWORD *)(Data + ofs);
		if (list[0] == langid)
			return NULL;
		ofsput = &list[2];
		ofs = list[2];
	}
	return ofsput;
}

DWORD FBehavior::FindLanguage (DWORD langid, bool ignoreregion) const
{
	BYTE *chunk;
	DWORD *list;
	DWORD langmask;

	langmask = ignoreregion ? ~LANGREGIONMASK : ~0;

	for (chunk = Chunks; chunk < Data + DataSize; chunk += ((DWORD *)chunk)[1] + 8)
	{
		list = (DWORD *)chunk;
		if (list[0] == MAKE_ID('S','T','R','L') && (list[2] & langmask) == langid)
		{
			return chunk - Data + 8;
		}
	}
	return 0;
}

void FBehavior::StartTypedScripts (WORD type, AActor *activator, int arg0, int arg1, int arg2, bool always) const
{
	if (!serverside)
		return;

	ScriptPtr *ptr;
	int i;

	for (i = 0; i < NumScripts; ++i)
	{
		ptr = (ScriptPtr *)(Scripts + 8*i);
		if (ptr->Type == type)
		{
			P_GetScriptGoing (activator, NULL, ptr->Number,
				(int *)(ptr->Address + Data), 0, arg0, arg1, arg2, always, true);
		}
	}
}

//---- The ACS Interpreter ----//



#define NEXTWORD	(LELONG(*pc++))
#define NEXTBYTE	(fmt==ACS_LittleEnhanced?getbyte(pc):NEXTWORD)
#define STACK(a)	(Stack[sp - (a)])
#define PushToStack(a)	(Stack[sp++] = (a))

void strbin (char *str);

IMPLEMENT_SERIAL (DACSThinker, DThinker)

DACSThinker *DACSThinker::ActiveThinker = NULL;

DACSThinker::DACSThinker ()
{
	if (ActiveThinker)
	{
		I_Error ("Only one ACSThinker is allowed to exist at a time.\nCheck your code.");
	}
	else
	{
		ActiveThinker = this;
		Scripts = NULL;
		LastScript = NULL;
		for (int i = 0; i < 1000; i++)
			RunningScripts[i] = NULL;
	}
}

DACSThinker::~DACSThinker ()
{
	DLevelScript *script = Scripts;
	while (script)
	{
		DLevelScript *next = script->next;
		script->Destroy ();
		script = next;
	}
	Scripts = NULL;
	ActiveThinker = NULL;
}

void DACSThinker::Serialize (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		arc << Scripts << LastScript;
		for (int i = 0; i < 1000; i++)
		{
			if (RunningScripts[i])
				arc << RunningScripts[i] << (WORD)i;
		}
		arc << (DLevelScript *)NULL;
	}
	else
	{
		arc >> Scripts >> LastScript;

		WORD scriptnum;
		DLevelScript *script;
		arc >> script;
		while (script)
		{
			arc >> scriptnum;
			RunningScripts[scriptnum] = script;
			arc >> script;
		}
	}
}

void DACSThinker::RunThink ()
{
	DLevelScript *script = Scripts;

	while (script)
	{
		DLevelScript *next = script->next;
		script->RunScript ();
		script = next;
	}
}

// FlashFader class - not sure where to put this so it goes here for now...
class DFlashFader : public DThinker
{
	DECLARE_SERIAL (DFlashFader, DThinker)
public:
	DFlashFader (float r1, float g1, float b1, float a1,
				 float r2, float g2, float b2, float a2,
				 float time, AActor *who);
	~DFlashFader ();
	virtual void RunThink ();
	virtual void DestroyedPointer(DObject *obj);
	AActor *WhoFor() { return ForWho; }
	void Cancel ();

protected:
	float Blends[2][4];
	int TotalTics;
	int StartTic;
	AActor *ForWho;

	void SetBlend (float time);
	DFlashFader ();
};

IMPLEMENT_SERIAL(DFlashFader, DThinker)

DFlashFader::DFlashFader ()
{
}

void DFlashFader::DestroyedPointer(DObject *obj)
{
	if(obj == ForWho)
		ForWho = NULL;
}

DFlashFader::DFlashFader (float r1, float g1, float b1, float a1,
						  float r2, float g2, float b2, float a2,
						  float time, AActor *who)
	: TotalTics ((int)(time*TICRATE)), StartTic (level.time), ForWho (who)
{
	Blends[0][0]=r1; Blends[0][1]=g1; Blends[0][2]=b1; Blends[0][3]=a1;
	Blends[1][0]=r2; Blends[1][1]=g2; Blends[1][2]=b2; Blends[1][3]=a2;
}

DFlashFader::~DFlashFader ()
{
	SetBlend (1.f);
}

void DFlashFader::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	if (arc.IsStoring ())
	{
		arc << TotalTics << StartTic << ForWho;

		for (int i = 1; i >= 0; --i)
			for (int j = 3; j >= 0; --j)
				arc << Blends[i][j];
	}
	else
	{
		arc >> TotalTics >> StartTic >> ForWho;

		for (int i = 1; i >= 0; --i)
			for (int j = 3; j >= 0; --j)
				arc >> Blends[i][j];
	}
}

void DFlashFader::RunThink ()
{
	if (ForWho == NULL || ForWho->player == NULL)
	{
		Destroy ();
		return;
	}
	if (level.time >= StartTic+TotalTics)
	{
		SetBlend (1.f);
		Destroy ();
		return;
	}
	SetBlend ((float)(level.time - StartTic) / (float)TotalTics);
}

void DFlashFader::SetBlend (float time)
{
	if (ForWho == NULL || ForWho->player == NULL)
		return;

	player_t* player = ForWho->player;

	float iT = 1.f - time;

	player->blend_color = fargb_t(
				Blends[0][3]*iT + Blends[1][3]*time,
				Blends[0][0]*iT + Blends[1][0]*time,
				Blends[0][1]*iT + Blends[1][1]*time,
				Blends[0][2]*iT + Blends[1][2]*time);
}

void DFlashFader::Cancel ()
{
	TotalTics = level.time - StartTic;
	Blends[1][3] = 0.f;
}

//---- Plane watchers ----//

class DPlaneWatcher : public DThinker
{
	DECLARE_SERIAL (DPlaneWatcher, DThinker)
public:
	DPlaneWatcher (AActor *it, line_t *line, int lineSide, bool ceiling,
		int tag, int height, int special,
		int arg0, int arg1, int arg2, int arg3, int arg4);
	virtual void RunThink ();
	virtual void DestroyedPointer(DObject *obj);
private:
	sector_t *Sector;
	fixed_t WatchD, LastD;
	int Special, Arg0, Arg1, Arg2, Arg3, Arg4;
	AActor *Activator;
	line_t *Line;
	int LineSide;
	bool bCeiling;

	DPlaneWatcher() {}
};

IMPLEMENT_SERIAL(DPlaneWatcher, DThinker)

void DPlaneWatcher::DestroyedPointer(DObject *obj)
{
	if(obj == Activator)
		Activator = NULL;
}

DPlaneWatcher::DPlaneWatcher (AActor *it, line_t *line, int lineSide, bool ceiling,
	int tag, int height, int special,
	int arg0, int arg1, int arg2, int arg3, int arg4)
	: Special (special), Arg0 (arg0), Arg1 (arg1), Arg2 (arg2), Arg3 (arg3), Arg4 (arg4),
	  Activator (it), Line (line), LineSide (lineSide), bCeiling (ceiling)
{
	int secnum;

	secnum = P_FindSectorFromTag (tag, -1);
	if (secnum >= 0)
	{
		Sector = &sectors[secnum];
		fixed_t amount = height << FRACBITS;
		if (bCeiling)
		{
			LastD = Sector->ceilingplane.d;
			P_ChangeCeilingHeight(Sector, amount);
			WatchD = Sector->ceilingplane.d;
			P_ChangeCeilingHeight(Sector, -amount);
		}
		else
		{
			LastD = Sector->floorplane.d;
			P_ChangeFloorHeight(Sector, amount);
			WatchD = Sector->floorplane.d;
			P_ChangeFloorHeight(Sector, -amount);
		}
	}
	else
	{
		Sector = NULL;
		WatchD = LastD = 0;
	}
}

void DPlaneWatcher::Serialize (FArchive &arc)
{
	Super::Serialize (arc);

	if (arc.IsStoring ())
	{
		arc << Special << Arg0 << Arg1 << Arg2 << Arg3 << Arg4
			<< Sector << bCeiling << WatchD << LastD << Activator
			<< Line << LineSide << bCeiling;
	}
	else
	{
		arc >> Special >> Arg0 >> Arg1 >> Arg2 >> Arg3 >> Arg4
			>> Sector >> bCeiling >> WatchD >> LastD >> Activator
			>> Line >> LineSide >> bCeiling;
	}
}

void DPlaneWatcher::RunThink ()
{
	if (Sector == NULL)
	{
		Destroy ();
		return;
	}

	fixed_t newd;

	if (bCeiling)
	{
		newd = Sector->ceilingplane.d;
	}
	else
	{
		newd = Sector->floorplane.d;
	}

	if ((LastD < WatchD && newd >= WatchD) ||
		(LastD > WatchD && newd <= WatchD))
	{
		TeleportSide = LineSide;
		LineSpecials[Special] (Line, Activator, Arg0, Arg1, Arg2, Arg3, Arg4);

		if (serverside)
		{
			SERVER_ONLY(SV_SendExecuteLineSpecial(Special, Line, Activator, Arg0, Arg1,
			                                      Arg2, Arg3, Arg4));
		}
		Destroy ();
	}
}


IMPLEMENT_SERIAL (DLevelScript, DObject)

void *DLevelScript::operator new (size_t size)
{
	return Z_Malloc (sizeof(DLevelScript), PU_LEVACS, 0);
}

void DLevelScript::operator delete (void *block)
{
	Z_Free (block);
}

void DLevelScript::Serialize (FArchive &arc)
{
	DWORD i;

	Super::Serialize (arc);

	if (arc.IsStoring ())
	{
		arc << next;
		arc << prev;
		arc << script;
		arc << sp;
		arc << state;
		arc << statedata;

		// [AM] We don't want player activators to be saved
		if (arc.IsReset() && P_ThinkerIsPlayerType(activator))
			arc << (AActor*)NULL;
		else
			arc << activator;

		arc << activationline;
		arc << lineSide;

		for (i = 0; i < LOCAL_SIZE; i++)
			arc << localvars[i];

		i = level.behavior->PC2Ofs(pc);
		arc << i;
	}
	else
	{
		arc >> next >> prev
			>> script
			>> sp
			>> state
			>> statedata
			>> activator
			>> activationline
			>> lineSide;

		for (i = 0; i < LOCAL_SIZE; i++)
			arc >> localvars[i];

		arc >> i;
		pc = level.behavior->Ofs2PC (i);
	}
}

DLevelScript::DLevelScript ()
{
	next = prev = NULL;
	if (DACSThinker::ActiveThinker == NULL)
		new DACSThinker;
}


void DLevelScript::Unlink ()
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

	if (controller->LastScript == this)
		controller->LastScript = prev;
	if (controller->Scripts == this)
		controller->Scripts = next;
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
}

void DLevelScript::Link ()
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

	next = controller->Scripts;
	if (controller->Scripts)
		controller->Scripts->prev = this;
	prev = NULL;
	controller->Scripts = this;
	if (controller->LastScript == NULL)
		controller->LastScript = this;
}

void DLevelScript::PutLast ()
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

	if (controller->LastScript == this)
		return;

	Unlink ();
	if (controller->Scripts == NULL)
	{
		Link ();
	}
	else
	{
		if (controller->LastScript)
			controller->LastScript->next = this;
		prev = controller->LastScript;
		next = NULL;
		controller->LastScript = this;
	}
}

void DLevelScript::PutFirst ()
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

	if (controller->Scripts == this)
		return;

	Unlink ();
	Link ();
}

int DLevelScript::Random (int min, int max)
{
	int num1, num2, num3, num4;
	unsigned int num;

	num1 = P_Random ();
	num2 = P_Random ();
	num3 = P_Random ();
	num4 = P_Random ();

	num = ((num1 << 24) | (num2 << 16) | (num3 << 8) | num4);
	num %= (max - min + 1);
	num += min;
	return (int)num;
}

int DLevelScript::ThingCount (int type, int tid)
{
	AActor *mobj = NULL;
	int count = 0;

	if (type >= NumSpawnableThings)
	{
		return 0;
	}
	else if (type > 0)
	{
		type = SpawnableThings[type];
		if (type == 0)
			return 0;
	}

	if (tid)
	{
		mobj = AActor::FindByTID (NULL, tid);
		while (mobj)
		{
			if ((type == 0) || (mobj->type == type && mobj->health > 0))
				count++;
			mobj = mobj->FindByTID (tid);
		}
	}
	else
	{
		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) )
		{
			if (type == 0)
			{
				count++;
			}
			else if (actor->type == type && actor->health > 0)
			{
				count++;
			}
		}
	}
	return count;
}

void DLevelScript::ChangeFlat (int tag, int name, bool floorOrCeiling)
{
	int secnum = -1;
	const OLumpName flatname = level.behavior->LookupString (name);

	if (flatname.empty())
		return;

	const int flat = R_FlatNumForName(flatname);

	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		if (floorOrCeiling == false)
		{
			sectors[secnum].floorpic = flat;
		}
		else
		{
			sectors[secnum].ceilingpic = flat;
		}
		sectors[secnum].SectorChanges |= SPC_FlatPic;
	}

	if (serverside)
	{
		const int argv[] = {tag, name};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(
		    SV_ACSExecuteSpecial(floorOrCeiling ? PCD_CHANGECEILING : PCD_CHANGEFLOOR,
		                         NULL, NULL, false, args));
	}
}

extern size_t P_NumPlayersInGame();

int DLevelScript::CountPlayers()
{
	return static_cast<int>(P_NumPlayersInGame());
}

void DLevelScript::ACS_SetLineTexture(int* args, byte argCount)
{
	if (argCount < 4)
		return;

	SetLineTexture(args[0], args[1], args[2], args[3]);
}

void DLevelScript::ACS_ClearInventory(AActor* actor)
{
	ClearInventory(actor);
}

void DLevelScript::ACS_Print(byte pcd, AActor* activator, const char* print)
{
	if (print == NULL)
		return;

	bool local = activator != NULL && pcd != DLevelScript::PCD_ENDPRINTBOLD;

	SERVER_ONLY(SV_ACSExecuteSpecial(pcd, activator, print, local));

	if (clientside &&
		(!local || (activator != NULL && activator->player && activator->player->mo == consoleplayer().camera)))
	{
		C_MidPrint(print);
	}
}

void DLevelScript::ACS_ChangeMusic(byte pcd, AActor* activator, int* args, byte argCount)
{
	if (argCount < 2)
		return;

	ChangeMusic(pcd, activator, args[0], args[1]);
}

void DLevelScript::ACS_StartSound(byte pcd, AActor* activator, int* args, byte argCount)
{
	if (pcd == PCD_SECTORSOUND)
	{
		if (argCount < 5)
			return;

		int sectorid = args[0];
		if (sectorid < numsectors)
			StartSectorSound(pcd, &sectors[sectorid], args[1], args[2], args[3], args[4]);
	}
	else if (pcd == PCD_THINGSOUND || pcd == PCD_ACTIVATORSOUND)
	{
		if (argCount < 4)
			return;

		StartThingSound(pcd, activator, args[0], args[1], args[2], args[3]);
	}
	else
	{
		if (argCount < 4)
			return;

		StartSound(pcd, activator, args[0], args[1], args[2], args[3]);
	}
}

void DLevelScript::ACS_SetLineBlocking(int* args, byte argCount)
{
	if (argCount < 2)
		return;

	SetLineBlocking(args[0], args[1]);
}

void DLevelScript::ACS_SetLineMonsterBlocking(int* args, byte argCount)
{
	if (argCount < 2)
		return;

	SetLineMonsterBlocking(args[0], args[1]);
}

void DLevelScript::ACS_SetLineSpecial(int* args, byte argCount)
{
	if (argCount < 7)
		return;

	SetLineSpecial(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
}

void DLevelScript::ACS_SetThingSpecial(int* args, byte argCount)
{
	if (argCount < 7)
		return;

	AActor* actor = P_FindThingById(args[0]);
	if (actor)
		SetThingSpecial(actor, args[1], args[2], args[3], args[4], args[5], args[6]);
}

void DLevelScript::ACS_FadeRange(AActor* activator, int* args, byte argCount)
{
	if (argCount < 9)
		return;

	DoFadeRange(activator, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
}

void DLevelScript::ACS_CancelFade(AActor* actor)
{
	CancelFade(actor);
}

void DLevelScript::ACS_ChangeFlat(byte pcd, int* args, byte argCount)
{
	if (argCount < 2)
		return;

	ChangeFlat(args[0], args[1], pcd == PCD_CHANGECEILING);
}

void DLevelScript::ACS_SoundSequence(int* args, byte argCount)
{
	if (argCount < 2)
		return;

	if (args[0] < numsectors)
		StartSoundSequence(&sectors[args[0]], args[1]);
}

void DLevelScript::SetLineTexture(int lineid, int side, int position, int name)
{
	int texture, linenum = -1;
	const OLumpName texname = level.behavior->LookupString(name);

	if (texname.empty())
		return;

	if (serverside)
	{
		int argv[] = {lineid, side, position, name};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SETLINETEXTURE, NULL, NULL, false, args));
	}

	side = (side) ? 1 : 0;

	texture = R_TextureNumForName(texname);

	while ((linenum = P_FindLineFromID (lineid, linenum)) >= 0)
	{
		side_t *currentSideDef;

		if (lines[linenum].sidenum[side] == R_NOSIDE)
			continue;

		lines[linenum].SidedefChanged = true;
		currentSideDef = sides + lines[linenum].sidenum[side];

		switch (position)
		{
			case TEXTURE_TOP:
				currentSideDef->toptexture = texture;
				currentSideDef->SidedefChanges |= SDPC_TexTop;
				break;
			case TEXTURE_MIDDLE:
				currentSideDef->midtexture = texture;
				currentSideDef->SidedefChanges |= SDPC_TexMid;
				break;
			case TEXTURE_BOTTOM:
				currentSideDef->bottomtexture = texture;
				currentSideDef->SidedefChanges |= SDPC_TexBottom;
				break;
			default:
				break;
		}
	}
}

void DLevelScript::SetLineBlocking(int lineid, int flags)
{
	int line = -1;

	while ((line = P_FindLineFromID(lineid, line)) >= 0)
	{
		switch (flags)
		{
		case BLOCK_NOTHING:
			lines[line].flags &= ~(ML_BLOCKING | ML_BLOCKEVERYTHING);
			lines[line].PropertiesChanged = true;
			break;
		case BLOCK_CREATURES:
		default:
			lines[line].flags &= ~ML_BLOCKEVERYTHING;
			lines[line].flags |= ML_BLOCKING;
			lines[line].PropertiesChanged = true;
			break;
		case BLOCK_EVERYTHING:
			lines[line].flags |= ML_BLOCKING | ML_BLOCKEVERYTHING;
			lines[line].PropertiesChanged = true;
			break;
		}
	}

	if (serverside)
	{
		int argv[] = {lineid, flags};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SETLINEBLOCKING, NULL, NULL, false, args));
	}
}

void DLevelScript::SetLineMonsterBlocking(int lineid, int toggle)
{
	int line = -1;

	while ((line = P_FindLineFromID(lineid, line)) >= 0)
	{
		if (toggle)
		{
			lines[line].flags |= ML_BLOCKMONSTERS;
		}
		else
		{
			lines[line].flags &= ~ML_BLOCKMONSTERS;
		}
	}

	if (serverside)
	{
		int argv[] = {lineid, toggle};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SETLINEMONSTERBLOCKING, NULL, NULL, false, args));
	}
}

void DLevelScript::SetLineSpecial(int lineid, int special, int arg1, int arg2, int arg3, int arg4, int arg5)
{
	int linenum = -1;

	while ((linenum = P_FindLineFromID(lineid, linenum)) >= 0)
	{
		line_t *line = &lines[linenum];

		line->special = special;
		line->args[0] = arg1;
		line->args[1] = arg2;
		line->args[2] = arg3;
		line->args[3] = arg4;
		line->args[4] = arg5;
	}

	if (serverside)
	{
		int argv[] = {lineid, special, arg1, arg2, arg3, arg4, arg5};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SETLINESPECIAL, NULL, NULL, false, args));
	}
}

void DLevelScript::ActivateLineSpecial(byte special, line_t* line, AActor* activator,
                                       int arg0, int arg1, int arg2, int arg3, int arg4)
{
	if (serverside)
	{
		LineSpecials[special](line, activator, arg0, arg1, arg2, arg3, arg4);
		SERVER_ONLY(SV_SendExecuteLineSpecial(special, line, activator, arg0, arg1, arg2,
		                                      arg3, arg4));
	}
}

void DLevelScript::ChangeMusic(byte pcd, AActor* activator, int index, int loop)
{
	bool local = (pcd == PCD_LOCALSETMUSIC || pcd == PCD_LOCALSETMUSICDIRECT);

	if (clientside)
	{
		if (local && activator != consoleplayer().mo)
			return;

		const char* lookup = level.behavior->LookupString(index);
		if (lookup != NULL)
			S_ChangeMusic(lookup, loop);
	}

	if (serverside)
	{
		int argv[] = {index, loop};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(pcd, activator, NULL, local, args));
	}
}


void DLevelScript::StartSound(byte pcd, AActor* activator, int channel, int index, int volume, int attenuation)
{
	bool local = pcd == PCD_LOCALAMBIENTSOUND;

	if (clientside)
	{
		if (local && consoleplayer().camera != activator)
			return;

		const char* lookup = level.behavior->LookupString(index);
		if (lookup != NULL)
			S_Sound(channel, lookup, (float)volume / 127.0F, attenuation);
	}

	if (serverside)
	{
		int argv[] = {channel, index, volume, attenuation};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(pcd, activator, NULL, local, args));
	}
}

void DLevelScript::StartSectorSound(byte pcd, sector_t* sector, int channel, int index, int volume, int attenuation)
{
	if (clientside)
	{
		const char* lookup = level.behavior->LookupString(index);
		if (lookup != NULL)
			S_Sound(sector->soundorg, channel, lookup, (float)volume / 127.0F, attenuation);
	}

	if (serverside)
	{
		int sectorNum = sector ? sector - sectors : 0;
		int argv[] = {sectorNum, channel, index, volume, attenuation};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(pcd, NULL, NULL, false, args));
	}
}

void DLevelScript::StartThingSound(byte pcd, AActor* actor, int channel, int index, int volume, int attenuation)
{
	if (clientside)
	{
		const char* lookup = level.behavior->LookupString(index);
		if (lookup != NULL)
			S_Sound(actor, channel, lookup, (float)volume / 127.0F, attenuation);
	}

	if (serverside)
	{
		int argv[] = {channel, index, volume, attenuation};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(pcd, actor, NULL, false, args));
	}
}

void DLevelScript::SetThingSpecial(AActor* actor, int special, int arg1, int arg2, int arg3, int arg4, int arg5)
{
	actor->special = special;
	actor->args[0] = arg1;
	actor->args[1] = arg2;
	actor->args[2] = arg3;
	actor->args[3] = arg4;
	actor->args[4] = arg5;

	if (serverside)
	{
		int argv[] = {static_cast<int>(actor->netid), special, arg1, arg2, arg3, arg4, arg5};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SETTHINGSPECIAL, actor, NULL, false, args));
	}
}

void DLevelScript::CancelFade(AActor* actor)
{
	if (clientside)
	{
		TThinkerIterator<DFlashFader> iterator;
		DFlashFader *fader;

		while ((fader = iterator.Next()))
		{
			if (actor == NULL || fader->WhoFor() == actor)
				fader->Cancel();
		}
	}

	SERVER_ONLY(SV_ACSExecuteSpecial(PCD_CANCELFADE, actor, NULL, true));
}

void DLevelScript::StartSoundSequence(sector_t* sec, int index)
{
	if (clientside)
	{
		const char* lookup = level.behavior->LookupString(index);
		if (lookup != NULL)
			SN_StartSequence(sec, lookup);
	}

	if (serverside)
	{
		int argv[] = {static_cast<int>(sec - sectors), index};
		std::vector<int> args = ArgvToArgs(argv);
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_SOUNDSEQUENCE, NULL, NULL, false, args));
	}
}

int DLevelScript::DoSpawn(int type, fixed_t x, fixed_t y, fixed_t z, int tid, int angle)
{
	const char* typestr = level.behavior->LookupString(type);
	if (typestr == NULL)
		return 0;
	char name[64];
	name[0] = 'A';
	name[63] = 0;
	strncpy(name+1, typestr, 62);

	//const TypeInfo* info = TypeInfo::FindType(name);
	mobjtype_t info;

	{
		// Find an entity through cycles
		info = FindDoomEntity(typestr, DoomMonsterNames, NUMMONSTERS);

		if (info == MT_NULL)
			info = FindWeaponEntity(typestr);
		if (info == MT_NULL)
			info = FindDoomEntity(typestr, DoomAmmoNames, 8);
		if (info == MT_NULL)
			info = FindDoomEntity(typestr, DoomKeyNames, 6);
		if (info == MT_NULL)
			info = FindDoomEntity(typestr, DoomPowerNames, 6);
		if (info == MT_NULL)
			info = FindDoomEntity(typestr, DoomHealthArmorNames, 9); // Ch0wW
		if (info == MT_NULL)
			info = FindDoomEntity(typestr, DoomDecorationNames, 60); // Ch0wW
	}

	AActor* actor = NULL;

	if (info != MT_NULL)
	{
		actor = new AActor (x, y, z, info);

		if (actor != NULL)
		{
			if (P_TestMobjLocation(actor))
			{
				actor->angle = angle << 24;
				actor->tid = tid;
				actor->AddToHash();
				actor->flags |= MF_DROPPED;  // Don't respawn
			}
			else
			{
				actor->Destroy();
				actor = NULL;
			}
		}
	}

	return (int)reinterpret_cast<uintptr_t>(actor);
}

int DLevelScript::DoSpawnSpot(int type, int spot, int tid, int angle)
{
	FActorIterator iterator(tid);
	AActor* aspot;
	int spawned = 0;

	while ((aspot = iterator.Next())) {
		spawned = DoSpawn(type, aspot->x, aspot->y, aspot->z, tid, angle);
	}
	return spawned;
}

void DLevelScript::DoFadeTo(AActor* who, int r, int g, int b, int a, fixed_t time)
{
    DPrintf("DoFadeRange now... \n");
	DoFadeRange(who, 0, 0, 0, -1, r, g, b, a, time);
}

static void DoActualFadeRange(player_s* viewer, float ftime, bool fadingFrom,
                              float fr1, float fg1, float fb1, float fa1,
                              float fr2, float fg2, float fb2, float fa2)
{
	if (ftime <= 0.f)
	{
		viewer->blend_color = fargb_t(fa2, fr2, fg2, fb2);
	}
	else
	{
		if (!fadingFrom)
		{
			if (viewer->blend_color.geta() == 0)
			{
				fr1 = fr2;
				fg1 = fg2;
				fb1 = fb2;
				fa1 = 0.f;
			}
			else
			{
				fr1 = viewer->blend_color.geta() / 255.0f;
				fr1 = viewer->blend_color.getr() / 255.0f;
				fg1 = viewer->blend_color.getg() / 255.0f;
				fb1 = viewer->blend_color.getb() / 255.0f;
			}
		}
		new DFlashFader(fr1, fg1, fb1, fa1, fr2, fg2, fb2, fa2, ftime, viewer->mo);
	}
}

void DLevelScript::DoFadeRange(AActor* who, int r1, int g1, int b1, int a1,
	int r2, int g2, int b2, int a2, fixed_t time)
{
	if (clientside && who->player != NULL)
	{
		float ftime = (float)time / 65536.f;
		bool fadingFrom = a1 >= 0;
		float fr1 = 0.f, fg1 = 0.f, fb1 = 0.f, fa1 = 0.f;
		float fr2, fg2, fb2, fa2;

		fr2 = (float)r2 / 255.f;
		fg2 = (float)g2 / 255.f;
		fb2 = (float)b2 / 255.f;
		fa2 = (float)a2 / 65536.f;

		if (fadingFrom)
		{
			fr1 = (float)r1 / 255.f;
			fg1 = (float)g1 / 255.f;
			fb1 = (float)b1 / 255.f;
			fa1 = (float)a1 / 65536.f;
		}

		DoActualFadeRange(who->player, ftime, fadingFrom, fr1, fg1, fb1, fa1, fr2, fg2, fb2, fa2);
	}

	if (serverside)
	{
		int argv[] = {r1, g1, b1, a1, r2, g2, b2, a2, time};
		std::vector<int> args(argv, argv + ARRAY_LENGTH(argv));
		SERVER_ONLY(SV_ACSExecuteSpecial(PCD_FADERANGE, who, NULL, true, args));
	}
}

inline int getbyte (int *&pc)
{
	int res = *(BYTE *)pc;
	pc = (int *)((BYTE *)pc+1);
	return res;
}

void DLevelScript::RunScript ()
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

    TeleportSide = lineSide;
    int *locals = localvars;
    ScriptFunction *activeFunction = NULL;

	switch (state)
	{
	case SCRIPT_Delayed:
		// Decrement the delay counter and enter state running
		// if it hits 0
		if (--statedata == 0)
			state = SCRIPT_Running;
		break;

	case SCRIPT_TagWait:
		// Wait for tagged sector(s) to go inactive, then enter
		// state running
	{
		int secnum = -1;

		while ((secnum = P_FindSectorFromTag (statedata, secnum)) >= 0)
			if (sectors[secnum].floordata || sectors[secnum].ceilingdata)
				return;

		// If we got here, none of the tagged sectors were busy
		state = SCRIPT_Running;
	}
	break;

	case SCRIPT_PolyWait:
		// Wait for polyobj(s) to stop moving, then enter state running
		if (!PO_Busy (statedata))
		{
			state = SCRIPT_Running;
		}
		break;

	case SCRIPT_ScriptWaitPre:
		// Wait for a script to start running, then enter state scriptwait
		if (controller->RunningScripts[statedata])
			state = SCRIPT_ScriptWait;
		break;

	case SCRIPT_ScriptWait:
		// Wait for a script to stop running, then enter state running
		if (controller->RunningScripts[statedata])
			return;

		state = SCRIPT_Running;
		PutFirst ();
		break;

	default:
		break;
	}

	int *pc = this->pc;
	int sp = this->sp;
	const ACSFormat fmt = level.behavior->GetFormat();
	int runaway = 0;	// used to prevent infinite loops
	int pcd;
	char work[4096], *workwhere = work;
	const char *lookup;
//	int optstart = -1;
	int temp;

	while (state == SCRIPT_Running)
	{
		if (++runaway > 500000)
		{
			DPrintf ("Runaway script %d terminated\n", script);
			state = SCRIPT_PleaseRemove;
			break;
		}

		pcd = NEXTBYTE;
		switch (pcd)
		{
		default:
			DPrintf("Unknown P-Code %d in script %d\n", pcd, script);
			continue;
			// fall through
		case PCD_TERMINATE:
			state = SCRIPT_PleaseRemove;
			break;

		case PCD_NOP:
			break;

		case PCD_SUSPEND:
			state = SCRIPT_Suspended;
			break;

		case PCD_PUSHNUMBER:
			PushToStack(NEXTWORD);
			break;

		case PCD_PUSHBYTE:
			PushToStack(*(BYTE*)pc);
			pc = (int*)((BYTE*)pc + 1);
			break;

		case PCD_PUSH2BYTES:
			Stack[sp] = ((BYTE*)pc)[0];
			Stack[sp + 1] = ((BYTE*)pc)[1];
			sp += 2;
			pc = (int*)((BYTE*)pc + 2);
			break;

		case PCD_PUSH3BYTES:
			Stack[sp] = ((BYTE*)pc)[0];
			Stack[sp + 1] = ((BYTE*)pc)[1];
			Stack[sp + 2] = ((BYTE*)pc)[2];
			sp += 3;
			pc = (int*)((BYTE*)pc + 3);
			break;

		case PCD_PUSH4BYTES:
			Stack[sp] = ((BYTE*)pc)[0];
			Stack[sp + 1] = ((BYTE*)pc)[1];
			Stack[sp + 2] = ((BYTE*)pc)[2];
			Stack[sp + 3] = ((BYTE*)pc)[3];
			sp += 4;
			pc = (int*)((BYTE*)pc + 4);
			break;

		case PCD_PUSH5BYTES:
			Stack[sp] = ((BYTE*)pc)[0];
			Stack[sp + 1] = ((BYTE*)pc)[1];
			Stack[sp + 2] = ((BYTE*)pc)[2];
			Stack[sp + 3] = ((BYTE*)pc)[3];
			Stack[sp + 4] = ((BYTE*)pc)[4];
			sp += 5;
			pc = (int*)((BYTE*)pc + 5);
			break;

		case PCD_PUSHBYTES:
			temp = *(BYTE*)pc;
			pc = (int*)((BYTE*)pc + temp + 1);
			for (temp = -temp; temp; temp++)
			{
				PushToStack(*((BYTE*)pc + temp));
			}
			break;

		case PCD_DUP:
			Stack[sp] = Stack[sp - 1];
			sp++;
			break;

		case PCD_SWAP:
			std::swap(Stack[sp - 2], Stack[sp - 1]);
			break;

		case PCD_LSPEC1:
			ActivateLineSpecial(NEXTBYTE, activationline, activator,
						STACK(1), 0, 0, 0, 0);
			sp -= 1;
			break;

		case PCD_LSPEC2:
			ActivateLineSpecial(NEXTBYTE, activationline, activator,
						STACK(2), STACK(1), 0, 0, 0);
			sp -= 2;
			break;

		case PCD_LSPEC3:
			ActivateLineSpecial(NEXTBYTE, activationline, activator,
						STACK(3), STACK(2), STACK(1), 0, 0);
			sp -= 3;
			break;

		case PCD_LSPEC4:
			ActivateLineSpecial(NEXTBYTE, activationline, activator,
						STACK(4), STACK(3), STACK(2),
						STACK(1), 0);
			sp -= 4;
			break;

		case PCD_LSPEC5:
			ActivateLineSpecial(NEXTBYTE, activationline, activator,
						STACK(5), STACK(4), STACK(3),
						STACK(2), STACK(1));
			sp -= 5;
			break;

		case PCD_LSPEC1DIRECT:
			temp = NEXTBYTE;
			ActivateLineSpecial(temp, activationline, activator,
						pc[0], 0, 0, 0, 0);
			pc += 1;
			break;

		case PCD_LSPEC2DIRECT:
			temp = NEXTBYTE;
			ActivateLineSpecial(temp, activationline, activator,
						pc[0], pc[1], 0, 0, 0);
			pc += 2;
			break;

		case PCD_LSPEC3DIRECT:
			temp = NEXTBYTE;
			ActivateLineSpecial(temp, activationline, activator,
						pc[0], pc[1], pc[2], 0, 0);
			pc += 3;
			break;

		case PCD_LSPEC4DIRECT:
			temp = NEXTBYTE;
			ActivateLineSpecial(temp, activationline, activator,
						pc[0], pc[1], pc[2], pc[3], 0);
			pc += 4;
			break;

		case PCD_LSPEC5DIRECT:
			temp = NEXTBYTE;
			ActivateLineSpecial(temp, activationline, activator,
						pc[0], pc[1], pc[2], pc[3], pc[4]);
			pc += 5;
			break;

		case PCD_LSPEC1DIRECTB:
			ActivateLineSpecial(((BYTE *)pc)[0], activationline, activator,
				((BYTE *)pc)[1], 0, 0, 0, 0);
			pc = (int *)((BYTE *)pc + 2);
			break;

		case PCD_LSPEC2DIRECTB:
			ActivateLineSpecial(((BYTE *)pc)[0], activationline, activator,
				((BYTE *)pc)[1], ((BYTE *)pc)[2], 0, 0, 0);
			pc = (int *)((BYTE *)pc + 3);
			break;

		case PCD_LSPEC3DIRECTB:
			ActivateLineSpecial(((BYTE *)pc)[0], activationline, activator,
				((BYTE *)pc)[1], ((BYTE *)pc)[2], ((BYTE *)pc)[3], 0, 0);
			pc = (int *)((BYTE *)pc + 4);
			break;

		case PCD_LSPEC4DIRECTB:
			ActivateLineSpecial(((BYTE *)pc)[0], activationline, activator,
				((BYTE *)pc)[1], ((BYTE *)pc)[2], ((BYTE *)pc)[3],
				((BYTE *)pc)[4], 0);
			pc = (int *)((BYTE *)pc + 5);
			break;

		case PCD_LSPEC5DIRECTB:
			ActivateLineSpecial(((BYTE *)pc)[0], activationline, activator,
				((BYTE *)pc)[1], ((BYTE *)pc)[2], ((BYTE *)pc)[3],
				((BYTE *)pc)[4], ((BYTE *)pc)[5]);
			pc = (int *)((BYTE *)pc + 6);
			break;

		case PCD_CALL:
		case PCD_CALLDISCARD: {
			int funcnum;
			int i;
			ScriptFunction* func;

			funcnum = NEXTBYTE;
			func = level.behavior->GetFunction(funcnum);
			if (func == NULL)
			{
				Printf(PRINT_HIGH, "Function %d in script %d out of range\n", funcnum,
				       script);
				state = SCRIPT_PleaseRemove;
				break;
			}
			if (sp + func->LocalCount + 32 > STACK_SIZE)
			{ // 32 is the margin for the function's working space
				Printf(PRINT_HIGH, "Out of stack space in script %d\n", script);
				state = SCRIPT_PleaseRemove;
				break;
			}
			// The function's first argument is also its first local variable.
			locals = &Stack[sp - func->ArgCount];
			// Make space on the stack for any other variables the function uses.
			for (i = 0; i < func->LocalCount; ++i)
			{
				Stack[sp + i] = 0;
			}
			sp += i;
			((CallReturn*)&Stack[sp])->ReturnAddress = level.behavior->PC2Ofs(pc);
			((CallReturn*)&Stack[sp])->ReturnFunction = activeFunction;
			((CallReturn*)&Stack[sp])->bDiscardResult = (pcd == PCD_CALLDISCARD);
			sp += sizeof(CallReturn) / sizeof(int);
			pc = level.behavior->Ofs2PC(func->Address);
			activeFunction = func;
		}
		break;

		case PCD_RETURNVOID:
		case PCD_RETURNVAL: {
			int value;
			CallReturn* retState;

			if (pcd == PCD_RETURNVAL)
			{
				value = Stack[--sp];
			}
			else
			{
				value = 0;
			}
			sp -= sizeof(CallReturn) / sizeof(int);
			retState = (CallReturn*)&Stack[sp];
			pc = level.behavior->Ofs2PC(retState->ReturnAddress);
			sp -= activeFunction->ArgCount + activeFunction->LocalCount;
			activeFunction = retState->ReturnFunction;
			if (activeFunction == NULL)
			{
				locals = localvars;
			}
			else
			{
				locals =
				    &Stack[sp - activeFunction->ArgCount - activeFunction->LocalCount];
			}
			if (!retState->bDiscardResult)
			{
				Stack[sp++] = value;
			}
		}
		break;

		case PCD_ADD:
			STACK(2) = STACK(2) + STACK(1);
			sp--;
			break;

		case PCD_SUBTRACT:
			STACK(2) = STACK(2) - STACK(1);
			sp--;
			break;

		case PCD_MULTIPLY:
			STACK(2) = STACK(2) * STACK(1);
			sp--;
			break;

		case PCD_DIVIDE:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				STACK(2) = STACK(2) / STACK(1);
				sp--;
			}
			break;

		case PCD_MODULUS:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				STACK(2) = STACK(2) % STACK(1);
				sp--;
			}
			break;

		case PCD_EQ:
			STACK(2) = (STACK(2) == STACK(1));
			sp--;
			break;

		case PCD_NE:
			STACK(2) = (STACK(2) != STACK(1));
			sp--;
			break;

		case PCD_LT:
			STACK(2) = (STACK(2) < STACK(1));
			sp--;
			break;

		case PCD_GT:
			STACK(2) = (STACK(2) > STACK(1));
			sp--;
			break;

		case PCD_LE:
			STACK(2) = (STACK(2) <= STACK(1));
			sp--;
			break;

		case PCD_GE:
			STACK(2) = (STACK(2) >= STACK(1));
			sp--;
			break;

		case PCD_ASSIGNSCRIPTVAR:
			locals[NEXTBYTE] = STACK(1);
			sp--;
			break;

		case PCD_ASSIGNMAPVAR:
			level.vars[NEXTBYTE] = STACK(1);
			sp--;
			break;

		case PCD_ASSIGNWORLDVAR:
			ACS_WorldVars[NEXTBYTE] = STACK(1);
			sp--;
			break;

		case PCD_ASSIGNGLOBALVAR:
			ACS_GlobalVars[NEXTBYTE] = STACK(1);
			sp--;
			break;

		case PCD_ASSIGNMAPARRAY:
			level.behavior->SetArrayVal(level.vars[NEXTBYTE], STACK(2), STACK(1));
			sp -= 2;
			break;

		case PCD_ASSIGNWORLDARRAY:
			ACS_WorldArrays[NEXTBYTE][STACK(2)] = STACK(1);
			sp -= 2;
			break;

		case PCD_ASSIGNGLOBALARRAY:
			ACS_GlobalArrays[NEXTBYTE][STACK(2)] = STACK(1);
			sp -= 2;
			break;

		case PCD_PUSHSCRIPTVAR:
			PushToStack(locals[NEXTBYTE]);
			break;

		case PCD_PUSHMAPVAR:
			PushToStack(level.vars[NEXTBYTE]);
			break;

		case PCD_PUSHWORLDVAR:
			PushToStack(ACS_WorldVars[NEXTBYTE]);
			break;

		case PCD_PUSHGLOBALVAR:
			PushToStack(ACS_GlobalVars[NEXTBYTE]);
			break;

		case PCD_PUSHMAPARRAY:
			STACK(1) = level.behavior->GetArrayVal(level.vars[NEXTBYTE], STACK(1));
			break;

		case PCD_PUSHWORLDARRAY:
			STACK(1) = ACS_WorldArrays[NEXTBYTE][STACK(1)];
			break;

		case PCD_PUSHGLOBALARRAY:
			STACK(1) = ACS_GlobalArrays[NEXTBYTE][STACK(1)];
			break;

		case PCD_ADDSCRIPTVAR:
			locals[NEXTBYTE] += STACK(1);
			sp--;
			break;

		case PCD_ADDMAPVAR:
			level.vars[NEXTBYTE] += STACK(1);
			sp--;
			break;

		case PCD_ADDWORLDVAR:
			ACS_WorldVars[NEXTBYTE] += STACK(1);
			sp--;
			break;

		case PCD_ADDGLOBALVAR:
			ACS_GlobalVars[NEXTBYTE] += STACK(1);
			sp--;
			break;

		case PCD_ADDMAPARRAY: {
			int a = level.vars[NEXTBYTE];
			int i = STACK(2);
			level.behavior->SetArrayVal(a, i,
			                            level.behavior->GetArrayVal(a, i) + STACK(1));
			sp -= 2;
		}
		break;

		case PCD_ADDWORLDARRAY:
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(2)] += STACK(1);
				sp -= 2;
			}
			break;

		case PCD_ADDGLOBALARRAY:
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(2)] += STACK(1);
				sp -= 2;
			}
			break;

		case PCD_SUBSCRIPTVAR:
			locals[NEXTBYTE] -= STACK(1);
			sp--;
			break;

		case PCD_SUBMAPVAR:
			level.vars[NEXTBYTE] -= STACK(1);
			sp--;
			break;

		case PCD_SUBWORLDVAR:
			ACS_WorldVars[NEXTBYTE] -= STACK(1);
			sp--;
			break;

		case PCD_SUBGLOBALVAR:
			ACS_GlobalVars[NEXTBYTE] -= STACK(1);
			sp--;
			break;

		case PCD_SUBMAPARRAY: {
			int a = level.vars[NEXTBYTE];
			int i = STACK(2);
			level.behavior->SetArrayVal(a, i,
			                            level.behavior->GetArrayVal(a, i) - STACK(1));
			sp -= 2;
		}
		break;

		case PCD_SUBWORLDARRAY:
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(2)] -= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_SUBGLOBALARRAY:
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(2)] -= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_MULSCRIPTVAR:
			locals[NEXTBYTE] *= STACK(1);
			sp--;
			break;

		case PCD_MULMAPVAR:
			level.vars[NEXTBYTE] *= STACK(1);
			sp--;
			break;

		case PCD_MULWORLDVAR:
			ACS_WorldVars[NEXTBYTE] *= STACK(1);
			sp--;
			break;

		case PCD_MULGLOBALVAR:
			ACS_GlobalVars[NEXTBYTE] *= STACK(1);
			sp--;
			break;

		case PCD_MULMAPARRAY: {
			int a = level.vars[NEXTBYTE];
			int i = STACK(2);
			level.behavior->SetArrayVal(a, i,
			                            level.behavior->GetArrayVal(a, i) * STACK(1));
			sp -= 2;
		}
		break;

		case PCD_MULWORLDARRAY:
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(2)] *= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_MULGLOBALARRAY:
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(2)] *= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_DIVSCRIPTVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				locals[NEXTBYTE] /= STACK(1);
				sp--;
			}
			break;

		case PCD_DIVMAPVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				level.vars[NEXTBYTE] /= STACK(1);
				sp--;
			}
			break;

		case PCD_DIVWORLDVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				ACS_WorldVars[NEXTBYTE] /= STACK(1);
				sp--;
			}
			break;

		case PCD_DIVGLOBALVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				ACS_GlobalVars[NEXTBYTE] /= STACK(1);
				sp--;
			}
			break;

		case PCD_DIVMAPARRAY:
			{
				if (STACK(1) == 0)
				{
					state = SCRIPT_DivideBy0;
				}
			    else
			    {
				    int a = level.vars[NEXTBYTE];
				    int i = STACK(2);
				    level.behavior->SetArrayVal(
				        a, i, level.behavior->GetArrayVal(a, i) / STACK(1));
				    sp -= 2;
			    }
			}
			break;

		case PCD_DIVWORLDARRAY:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(2)] /= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_DIVGLOBALARRAY:
			if (STACK(1) == 0)
			{
				state = SCRIPT_DivideBy0;
			}
			else
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(2)] /= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_MODSCRIPTVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				locals[NEXTBYTE] %= STACK(1);
				sp--;
			}
			break;

		case PCD_MODMAPVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				level.vars[NEXTBYTE] %= STACK(1);
				sp--;
			}
			break;

		case PCD_MODWORLDVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				ACS_WorldVars[NEXTBYTE] %= STACK(1);
				sp--;
			}
			break;

		case PCD_MODGLOBALVAR:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				ACS_GlobalVars[NEXTBYTE] %= STACK(1);
				sp--;
			}
			break;

		case PCD_MODMAPARRAY:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				int a = level.vars[NEXTBYTE];
				int i = STACK(2);
				level.behavior->SetArrayVal(a, i,
											level.behavior->GetArrayVal(a, i) % STACK(1));
				sp -= 2;
			}
			break;

		case PCD_MODWORLDARRAY:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(2)] %= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_MODGLOBALARRAY:
			if (STACK(1) == 0)
			{
				state = SCRIPT_ModulusBy0;
			}
			else
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(2)] %= STACK(1);
				sp -= 2;
			}
			break;

		case PCD_INCSCRIPTVAR:
			++locals[NEXTBYTE];
			break;

		case PCD_INCMAPVAR:
			++level.vars[NEXTBYTE];
			break;

		case PCD_INCWORLDVAR:
			++ACS_WorldVars[NEXTBYTE];
			break;

		case PCD_INCGLOBALVAR:
			++ACS_GlobalVars[NEXTBYTE];
			break;

		case PCD_INCMAPARRAY:
			{
				int a = level.vars[NEXTBYTE];
				int i = STACK(2);
				level.behavior->SetArrayVal (a, i,
					level.behavior->GetArrayVal (a, i) + 1);
				sp--;
			}
			break;

		case PCD_INCWORLDARRAY:
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(1)] += 1;
				sp--;
			}
			break;

		case PCD_INCGLOBALARRAY:
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(1)] += 1;
				sp--;
			}
			break;

		case PCD_DECSCRIPTVAR:
			--locals[NEXTBYTE];
			break;

		case PCD_DECMAPVAR:
			--level.vars[NEXTBYTE];
			break;

		case PCD_DECWORLDVAR:
			--ACS_WorldVars[NEXTBYTE];
			break;

		case PCD_DECGLOBALVAR:
			--ACS_GlobalVars[NEXTBYTE];
			break;

		case PCD_DECMAPARRAY:
			{
				int a = level.vars[NEXTBYTE];
				int i = STACK(2);
				level.behavior->SetArrayVal (a, i,
					level.behavior->GetArrayVal (a, i) - 1);
				sp--;
			}
			break;

		case PCD_DECWORLDARRAY:
			{
				int a = NEXTBYTE;
				ACS_WorldArrays[a][STACK(1)] -= 1;
				sp--;
			}
			break;

		case PCD_DECGLOBALARRAY:
			{
				int a = NEXTBYTE;
				ACS_GlobalArrays[a][STACK(1)] -= 1;
				sp--;
			}
			break;

		case PCD_GOTO:
			pc = level.behavior->Ofs2PC (*pc);
			break;

		case PCD_IFGOTO:
			if (STACK(1))
				pc = level.behavior->Ofs2PC (*pc);
			else
				pc++;
			sp--;
			break;

		case PCD_DROP:
			sp--;
			break;

		case PCD_DELAY:
			state = SCRIPT_Delayed;
			statedata = STACK(1);
			sp--;
			break;

		case PCD_DELAYDIRECT:
			state = SCRIPT_Delayed;
			statedata = NEXTWORD;
			break;

		case PCD_DELAYDIRECTB:
			state = SCRIPT_Delayed;
			statedata = *(BYTE *)pc;
			pc = (int *)((BYTE *)pc + 1);
			break;

		case PCD_RANDOM:
			STACK(2) = Random (STACK(2), STACK(1));
			sp--;
			break;

		case PCD_RANDOMDIRECT:
			PushToStack (Random (pc[0], pc[1]));
			pc += 2;
			break;

		case PCD_RANDOMDIRECTB:
			PushToStack (Random (((BYTE *)pc)[0], ((BYTE *)pc)[1]));
			pc = (int *)((BYTE *)pc + 2);
			break;

		case PCD_THINGCOUNT:
			STACK(2) = ThingCount (STACK(2), STACK(1));
			sp--;
			break;

		case PCD_THINGCOUNTDIRECT:
			PushToStack (ThingCount (pc[0], pc[1]));
			pc += 2;
			break;

		case PCD_TAGWAIT:
			state = SCRIPT_TagWait;
			statedata = STACK(1);
			sp--;
			break;

		case PCD_TAGWAITDIRECT:
			state = SCRIPT_TagWait;
			statedata = NEXTWORD;
			break;

		case PCD_POLYWAIT:
			state = SCRIPT_PolyWait;
			statedata = STACK(1);
			sp--;
			break;

		case PCD_POLYWAITDIRECT:
			state = SCRIPT_PolyWait;
			statedata = NEXTWORD;
			break;

		case PCD_CHANGEFLOOR:
			ChangeFlat (STACK(2), STACK(1), 0);
			sp -= 2;
			break;

		case PCD_CHANGEFLOORDIRECT:
			ChangeFlat (pc[0], pc[1], 0);
			pc += 2;
			break;

		case PCD_CHANGECEILING:
			ChangeFlat (STACK(2), STACK(1), 1);
			sp -= 2;
			break;

		case PCD_CHANGECEILINGDIRECT:
			ChangeFlat (pc[0], pc[1], 1);
			pc += 2;
			break;

		case PCD_RESTART:
			pc = level.behavior->FindScript (script);
			break;

		case PCD_ANDLOGICAL:
			STACK(2) = (STACK(2) && STACK(1));
			sp--;
			break;

		case PCD_ORLOGICAL:
			STACK(2) = (STACK(2) || STACK(1));
			sp--;
			break;

		case PCD_ANDBITWISE:
			STACK(2) = (STACK(2) & STACK(1));
			sp--;
			break;

		case PCD_ORBITWISE:
			STACK(2) = (STACK(2) | STACK(1));
			sp--;
			break;

		case PCD_EORBITWISE:
			STACK(2) = (STACK(2) ^ STACK(1));
			sp--;
			break;

		case PCD_NEGATELOGICAL:
			STACK(1) = !STACK(1);
			break;

		case PCD_LSHIFT:
			STACK(2) = (STACK(2) << STACK(1));
			sp--;
			break;

		case PCD_RSHIFT:
			STACK(2) = (STACK(2) >> STACK(1));
			sp--;
			break;

		case PCD_UNARYMINUS:
			STACK(1) = -STACK(1);
			break;

		case PCD_IFNOTGOTO:
			if (!STACK(1))
				pc = level.behavior->Ofs2PC (*pc);
			else
				pc++;
			sp--;
			break;

		case PCD_LINESIDE:
			PushToStack (lineSide);
			break;

		case PCD_SCRIPTWAIT:
			statedata = STACK(1);
			if (controller->RunningScripts[statedata])
				state = SCRIPT_ScriptWait;
			else
				state = SCRIPT_ScriptWaitPre;
			sp--;
			PutLast ();
			break;

		case PCD_SCRIPTWAITDIRECT:
			state = SCRIPT_ScriptWait;
			statedata = NEXTWORD;
			PutLast ();
			break;

		case PCD_CLEARLINESPECIAL:
			if (activationline)
				activationline->special = 0;
			break;

		case PCD_CASEGOTO:
			if (STACK(1) == NEXTWORD)
			{
				pc = level.behavior->Ofs2PC (*pc);
				sp--;
			}
			else
			{
				pc++;
			}
			break;

		case PCD_BEGINPRINT:
			workwhere = work;
			work[0] = 0;
			break;

		case PCD_PRINTSTRING:
		case PCD_PRINTLOCALIZED:
			lookup = (pcd == PCD_PRINTSTRING ?
				level.behavior->LookupString (STACK(1)) :
				level.behavior->LocalizeString (STACK(1)));
			if (lookup != NULL)
			{
				workwhere += snprintf(workwhere, 4096, "%s", lookup);
			}
			--sp;
			break;

		case PCD_PRINTNUMBER:
			workwhere += snprintf(workwhere, 4096, "%d", STACK(1));
			--sp;
			break;

		case PCD_PRINTCHARACTER:
			workwhere[0] = STACK(1);
			workwhere[1] = 0;
			workwhere++;
			--sp;
			break;

		case PCD_PRINTFIXED:
			workwhere += snprintf(workwhere, 4096, "%g", FIXED2FLOAT(STACK(1)));
			--sp;
			break;

		// [BC] Print activator's name
		// [RH] Fancied up a bit
		case PCD_PRINTNAME:
			{
				player_t *player = NULL;

				if (STACK(1) == 0 || (unsigned)STACK(1) > MAXPLAYERS)
				{
					if (activator)
					{
						player = activator->player;
					}
				}
				else if (idplayer(STACK(1)).ingame())
				{
					player = &idplayer(STACK(1));
				}
				else
				{
				    workwhere += snprintf(workwhere, 4096, "Player %d\n",
						STACK(1));
					sp--;
					break;
				}
				if (player)
				{
				    workwhere += snprintf(workwhere, 4096, "%s",
						activator->player->userinfo.netname.c_str());
				}
				else if (activator)
				{
				    workwhere += snprintf(workwhere, 4096, "%s",
						RUNTIME_TYPE(activator)->Name+1);
				}
				else
				{
				    workwhere += snprintf(workwhere, 4096, " ");
				}
				sp--;
			}
			break;

		case PCD_ENDPRINT:
		case PCD_ENDPRINTBOLD:
		//case PCD_MOREHUDMESSAGE:
			strbin (work);
			if (pcd != PCD_MOREHUDMESSAGE)
			{
				ACS_Print(pcd, activator, work);
			}
			else
			{
//				optstart = -1;
			}
			break;

		/*case PCD_OPTHUDMESSAGE:
			optstart = sp;
			break;

		case PCD_ENDHUDMESSAGE:
		case PCD_ENDHUDMESSAGEBOLD:
			if (optstart == -1)
			{
				optstart = sp;
			}
			if (pcd == PCD_ENDHUDMESSAGEBOLD || activator == NULL ||
				(activator->player->mo == consoleplayer().camera))
			{
				int type = Stack[optstart-6];
				int id = Stack[optstart-5];
				EColorRange color = CLAMPCOLOR(Stack[optstart-4]);
				float x = FIXED2FLOAT(Stack[optstart-3]);
				float y = FIXED2FLOAT(Stack[optstart-2]);
				float holdTime = FIXED2FLOAT(Stack[optstart-1]);
				DHUDMessage *msg;

				switch (type)
				{
				default:	// normal
					msg = new DHUDMessage (work, x, y, color, holdTime);
					break;
				case 1:		// fade out
					{
						float fadeTime = (optstart < sp) ?
							FIXED2FLOAT(Stack[optstart]) : 0.5f;
						msg = new DHUDMessageFadeOut (work, x, y, color, holdTime, fadeTime);
					}
					break;
				case 2:		// type on, then fade out
					{
						float typeTime = (optstart < sp) ?
							FIXED2FLOAT(Stack[optstart]) : 0.05f;
						float fadeTime = (optstart < sp-1) ?
							FIXED2FLOAT(Stack[optstart+1]) : 0.5f;
						msg = new DHUDMessageTypeOnFadeOut (work, x, y, color, typeTime, holdTime, fadeTime);
					}
					break;
				}
				StatusBar->AttachMessage (msg, id ? 0xff000000|id : 0);
			}
			sp = optstart-6;
			break;
        */
		/*case PCD_SETFONT:
			DoSetFont (STACK(1));
			sp--;
			break;

		case PCD_SETFONTDIRECT:
			DoSetFont (pc[0]);
			pc++;
			break;
        */
		case PCD_PLAYERCOUNT:
			PushToStack (CountPlayers ());
			break;

		case PCD_GAMETYPE:
		    if (sv_gametype == 3)
                PushToStack (GAME_NET_CTF);
            else if (sv_gametype == 2)
                PushToStack (GAME_NET_TEAMDEATHMATCH);
			else if (sv_gametype == 1)
				PushToStack (GAME_NET_DEATHMATCH);
			else if (multiplayer)
				PushToStack (GAME_NET_COOPERATIVE);
			else
				PushToStack (GAME_SINGLE_PLAYER);
			break;

		case PCD_GAMESKILL:
			PushToStack (sv_skill);
			break;

// [BC] Start ST PCD's
		case PCD_PLAYERHEALTH:
			if (activator)
				PushToStack (activator->health);
			else
				PushToStack (0);
			break;

		case PCD_PLAYERARMORPOINTS:
			if (activator && activator->player)
				PushToStack (activator->player->armorpoints);
			else
				PushToStack (0);
			break;

		case PCD_PLAYERFRAGS:
			if (activator && activator->player)
				PushToStack (activator->player->fragcount);
			else
				PushToStack (0);
			break;

		case PCD_MUSICCHANGE:
			ChangeMusic(pcd, activator, STACK(2), STACK(1));
			sp -= 2;
			break;

		case PCD_SINGLEPLAYER:
			PushToStack (!multiplayer);
			break;
// [BC] End ST PCD's

		case PCD_TIMER:
			PushToStack (level.time);
			break;

		case PCD_SECTORSOUND:
			if (activationline)
				StartSectorSound(pcd, activationline->frontsector, CHAN_BODY, STACK(2), STACK(1), ATTN_NORM);
			else
				StartSound(pcd, NULL, CHAN_BODY, STACK(2), STACK(1), ATTN_NORM);
			sp -= 2;
			break;

		case PCD_AMBIENTSOUND:
			StartSound(pcd, activator, CHAN_AUTO, STACK(2), STACK(1), ATTN_NONE);
			break;

		case PCD_LOCALAMBIENTSOUND:
			StartSound(pcd, activator, CHAN_AUTO, STACK(2), STACK(1), ATTN_NONE);
			sp -= 2;
			break;

		case PCD_ACTIVATORSOUND:
			StartThingSound(pcd, activator, CHAN_AUTO, STACK(2), STACK(1), ATTN_NORM);
			sp -= 2;
			break;

		case PCD_SOUNDSEQUENCE:
			if (activationline)
				StartSoundSequence(activationline->frontsector, STACK(1));
			sp--;
			break;

		case PCD_SETLINETEXTURE:
			SetLineTexture (STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 4;
			break;

		case PCD_SETLINEBLOCKING:
			SetLineBlocking(STACK(2), STACK(1));
			sp -= 2;
			break;

		case PCD_SETLINEMONSTERBLOCKING:
			SetLineMonsterBlocking(STACK(2), STACK(1));
			sp -= 2;
			break;

		case PCD_SETLINESPECIAL:
			SetLineSpecial(STACK(7), STACK(6), STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 7;
			break;

		case PCD_SETTHINGSPECIAL:
		{
			FActorIterator iterator (STACK(7));
			AActor *actor;

			while ( (actor = iterator.Next ()) )
				SetThingSpecial(actor, STACK(6), STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 7;
		}
			break;

		case PCD_THINGSOUND:
		{
			FActorIterator iterator(STACK(3));
			AActor *spot;
			while ((spot = iterator.Next()))
				StartThingSound(pcd, spot, CHAN_BODY, STACK(2), STACK(1), ATTN_NORM);
			sp -= 3;
		}
			break;

		case PCD_FIXEDMUL:
			STACK(2) = FixedMul (STACK(2), STACK(1));
			sp--;
			break;

		case PCD_FIXEDDIV:
			STACK(2) = FixedDiv (STACK(2), STACK(1));
			sp--;
			break;

		case PCD_SETGRAVITY:
			level.gravity = (float)STACK(1) / 65536.f;
			sp--;
			break;

		case PCD_SETGRAVITYDIRECT:
			level.gravity = (float)pc[0] / 65536.f;
			pc++;
			break;

		case PCD_SETAIRCONTROL:
			level.aircontrol = STACK(1);
			sp--;
			G_AirControlChanged ();
			break;

		case PCD_SETAIRCONTROLDIRECT:
			level.aircontrol = pc[0];
			pc++;
			G_AirControlChanged ();
			break;

		case PCD_SPAWN:
			STACK(6) = DoSpawn (STACK(6), STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 5;
			break;

		case PCD_SPAWNDIRECT:
			PushToStack (DoSpawn (pc[0], pc[1], pc[2], pc[3], pc[4], pc[5]));
			pc += 6;
			break;

		case PCD_SPAWNSPOT:
			STACK(4) = DoSpawnSpot (STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 3;
			break;

		case PCD_SPAWNSPOTDIRECT:
			PushToStack (DoSpawnSpot (pc[0], pc[1], pc[2], pc[3]));
			pc += 4;
			break;

		case PCD_CLEARINVENTORY:
			ClearInventory (activator);
			break;

		case PCD_GIVEINVENTORY:
			GiveInventory (activator, level.behavior->LookupString (STACK(2)), STACK(1));
			sp -= 2;
			break;

		case PCD_GIVEINVENTORYDIRECT:
			GiveInventory (activator, level.behavior->LookupString (pc[0]), pc[1]);
			pc += 2;
			break;

		case PCD_TAKEINVENTORY:
			TakeInventory (activator, level.behavior->LookupString (STACK(2)), STACK(1));
			sp -= 2;
			break;

		case PCD_TAKEINVENTORYDIRECT:
			TakeInventory (activator, level.behavior->LookupString (pc[0]), pc[1]);
			pc += 2;
			break;

		case PCD_CHECKINVENTORY:
			STACK(1) = CheckInventory (activator, level.behavior->LookupString (STACK(1)));
			break;

		case PCD_CHECKINVENTORYDIRECT:
			PushToStack (CheckInventory (activator, level.behavior->LookupString (pc[0])));
			pc += 1;
			break;

		case PCD_SETMUSIC:
			ChangeMusic(pcd, NULL, STACK(3), STACK(2));
			sp -= 3;
			break;

		case PCD_SETMUSICDIRECT:
			ChangeMusic(pcd, NULL, pc[0], pc[1]);
			pc += 3;
			break;

		case PCD_LOCALSETMUSIC:
			ChangeMusic(pcd, activator, STACK(3), STACK(2));
			sp -= 3;
			break;

		case PCD_LOCALSETMUSICDIRECT:
			ChangeMusic(pcd, activator, pc[0], pc[1]);
			pc += 3;
			break;

		case PCD_FADETO:
			DoFadeTo (activator, STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 5;
			break;

		case PCD_FADERANGE:
			DoFadeRange (activator, STACK(9), STACK(8), STACK(7), STACK(6),
						 STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 9;
			break;

		case PCD_CANCELFADE:
			CancelFade(activator);
			break;

		/*case PCD_PLAYMOVIE:
			STACK(1) = I_PlayMovie (level.behavior->LookupString (STACK(1)));
			break;
        */
		case PCD_GETACTORX:
		case PCD_GETACTORY:
		case PCD_GETACTORZ:
			{
			    AActor *actor = SingleActorFromTID(STACK(1), activator);

				if (actor == NULL)
				{
					STACK(1) = 0;
				}
				else
				{
					STACK(1) = (&actor->x)[pcd - PCD_GETACTORX];
				}
			}
			break;

		case PCD_GETACTORANGLE:
			{
				AActor *actor = SingleActorFromTID (STACK(1), activator);

				if (actor == NULL)
				{
					STACK(1) = 0;
				}
				else
				{
					STACK(1) = actor->angle >> FRACBITS;
				}
			}
			break;

		case PCD_SETFLOORTRIGGER:
			new DPlaneWatcher (activator, activationline, lineSide, false, STACK(8),
				STACK(7), STACK(6), STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 8;
			break;

		case PCD_SETCEILINGTRIGGER:
			new DPlaneWatcher (activator, activationline, lineSide, true, STACK(8),
				STACK(7), STACK(6), STACK(5), STACK(4), STACK(3), STACK(2), STACK(1));
			sp -= 8;
			break;

		/*case PCD_STARTTRANSLATION:
			{
				int i = STACK(1);
				sp--;
				if (i >= 1 && i <= MAX_ACS_TRANSLATIONS)
				{
					translation = &translationtables[TRANSLATION_LevelScripted][i*256-256];
					for (i = 0; i < 256; ++i)
					{
						translation[i] = i;
					}
				}
			}
			break;

		case PCD_TRANSLATIONRANGE1:
			{ // translation using palette shifting
				int start = STACK(4);
				int end = STACK(3);
				int pal1 = STACK(2);
				int pal2 = STACK(1);
				fixed_t palcol, palstep;
				sp -= 4;

				if (translation == NULL)
				{
					break;
				}
				if (start > end)
				{
					swap (start, end);
					swap (pal1, pal2);
				}
				else if (start == end)
				{
					translation[start] = pal1;
					break;
				}
				palcol = pal1 << FRACBITS;
				palstep = ((pal2 << FRACBITS) - palcol) / (end - start);
				for (int i = start; i <= end; palcol += palstep, ++i)
				{
					translation[i] = palcol >> FRACBITS;
				}
			}
			break;

		case PCD_TRANSLATIONRANGE2:
			{ // translation using RGB values
			  // (would HSV be a good idea too?)
				int start = STACK(8);
				int end = STACK(7);
				fixed_t r1 = STACK(6) << FRACBITS;
				fixed_t g1 = STACK(5) << FRACBITS;
				fixed_t b1 = STACK(4) << FRACBITS;
				fixed_t r2 = STACK(3) << FRACBITS;
				fixed_t g2 = STACK(2) << FRACBITS;
				fixed_t b2 = STACK(1) << FRACBITS;
				fixed_t r, g, b;
				fixed_t rs, gs, bs;
				sp -= 8;

				if (translation == NULL)
				{
					break;
				}
				if (start > end)
				{
					swap (start, end);
					r = r2;
					g = g2;
					b = b2;
					rs = r1 - r2;
					gs = g1 - g2;
					bs = b1 - b2;
				}
				else
				{
					r = r1;
					g = g1;
					b = b1;
					rs = r2 - r1;
					gs = g2 - g1;
					bs = b2 - b1;
				}
				if (start == end)
				{
					translation[start] = ColorMatcher.Pick
						(r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
					break;
				}
				rs /= (end - start);
				gs /= (end - start);
				bs /= (end - start);
				for (int i = start; i <= end; ++i)
				{
					translation[i] = ColorMatcher.Pick
						(r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
					r += rs;
					g += gs;
					b += bs;
				}
			}
			break;

		case PCD_ENDTRANSLATION:
			// This might be useful for hardware rendering, but
			// for software it is superfluous.
			translation = NULL;
			break;
        */

		case PCD_SIN:
			STACK(1) = finesine[(STACK(1)<<16)>>ANGLETOFINESHIFT];
			break;

		case PCD_COS:
			STACK(1) = finecosine[(STACK(1)<<16)>>ANGLETOFINESHIFT];
			break;

		case PCD_VECTORANGLE:
			STACK(2) = R_PointToAngle2 (0, 0, STACK(2), STACK(1)) >> 16;
			sp--;
			break;

		case PCD_PLAYERNUMBER:
			if (activator == NULL || activator->player == NULL)
				PushToStack(-1);
			else
				PushToStack(activator->player->GetPlayerNumber());
			break;

		case PCD_ACTIVATORTID:
			if (activator == NULL)
				PushToStack(0);
			else
				PushToStack(activator->tid);
			break;

		case PCD_GETCVAR: {
			cvar_t *var, *prev;
			var = cvar_t::FindCVar(level.behavior->LookupString(STACK(1)), &prev);
			if (var == NULL)
			{
				STACK(1) = 0;
			}
			else
			{
				STACK(1) = (int)var->value();
			}
		}
		break;

		case PCD_GETLEVELINFO:
			switch (STACK(1))
			{
			case LEVELINFO_PAR_TIME:
				STACK(1) = level.partime;
				break;
			case LEVELINFO_SUCK_TIME:
				STACK(1) = 0;	//level.sucktime; -- ToDo?
				break;
			case LEVELINFO_CLUSTERNUM:
				STACK(1) = level.cluster;
				break;
			case LEVELINFO_LEVELNUM:
				STACK(1) = level.levelnum;
				break;
			case LEVELINFO_TOTAL_SECRETS:
				STACK(1) = level.total_secrets;
				break;
			case LEVELINFO_FOUND_SECRETS:
				STACK(1) = level.found_secrets;
				break;
			case LEVELINFO_TOTAL_ITEMS:
				STACK(1) = level.total_items;
				break;
			case LEVELINFO_FOUND_ITEMS:
				STACK(1) = level.found_items;
				break;
			case LEVELINFO_TOTAL_MONSTERS:
				STACK(1) = level.total_monsters;
				break;
			case LEVELINFO_KILLED_MONSTERS:
				STACK(1) = level.killed_monsters;
				break;
			default:
				STACK(1) = 0;
				break;
			}
			break;

		/*case PCD_CHECKWEAPON:
			if (activator == NULL || activator->player == NULL)
			{ // Non-players do not have ready weapons
				STACK(1) = 0;
			}
			else
			{
				STACK(1) = 0 == strcmp (level.behavior->LookupString (STACK(1)),
					wpnlev1info[activator->player->readyweapon]->type->Name+1);
			}
			break;

		case PCD_SETWEAPON:
			if (activator == NULL || activator->player == NULL)
			{
				STACK(1) = 0;
			}
			else
			{
				int i;

				for (i = 0; i < NUMWEAPONS; ++i)
				{
					if (0 == strcmp (level.behavior->LookupString (STACK(1)),
						wpnlev1info[i]->type->Name+1))
					{
						break;
					}
				}
				if (i >= NUMWEAPONS || !activator->player->weaponowned[i])
				{
					STACK(1) = 0;
				}
				else
				{
					STACK(1) = 1;
					if (activator->player->readyweapon != i)
					{
						activator->player->pendingweapon = (weapontype_t)i;
					}
				}
			}
			break;
        */
		}
	}

	this->pc = pc;
	this->sp = sp;

	if (state == SCRIPT_DivideBy0)
	{
		DPrintf("Divide by zero in script %d\n", script);
		state = SCRIPT_PleaseRemove;
	}
	else if (state == SCRIPT_ModulusBy0)
	{
		DPrintf("Modulus by zero in script %d\n", script);
		state = SCRIPT_PleaseRemove;
	}

	if (state == SCRIPT_PleaseRemove)
	{
		Unlink ();
		if (!controller)
			return;

		if (controller->RunningScripts[script] == this)
			controller->RunningScripts[script] = NULL;
		this->Destroy ();
	}
}

static bool P_GetScriptGoing (AActor *who, line_t *where, int num, int *code,
	int lineSide, int arg0, int arg1, int arg2, int always, bool delay)
{
	DACSThinker *controller = DACSThinker::ActiveThinker;

	if (controller && !always && controller->RunningScripts[num])
	{
		if (controller->RunningScripts[num]->GetState () == DLevelScript::SCRIPT_Suspended)
		{
			controller->RunningScripts[num]->SetState (DLevelScript::SCRIPT_Running);
			return true;
		}
		return false;
	}

	new DLevelScript (who, where, num, code, lineSide, arg0, arg1, arg2, always, delay);

	return true;
}

DLevelScript::DLevelScript (AActor *who, line_t *where, int num, int *code, int lineside,
							int arg0, int arg1, int arg2, int always, bool delay)
{
	if (DACSThinker::ActiveThinker == NULL)
		new DACSThinker;

	script = num;
	sp = 0;
	localvars[0] = arg0;
	localvars[1] = arg1;
	localvars[2] = arg2;
	memset (localvars+3, 0, sizeof(localvars)-3*sizeof(int));
	pc = code;
	activator = who;
	activationline = where;
	lineSide = lineside;
	if (delay) {
		// From Hexen: Give the world some time to set itself up before
		// running open scripts.
		//script->state = SCRIPT_Delayed;
		//script->statedata = TICRATE;
		state = SCRIPT_Running;
	} else {
		state = SCRIPT_Running;
	}

	if (!always)
		DACSThinker::ActiveThinker->RunningScripts[num] = this;

	Link ();

	DPrintf ("Script %d started.\n", num);
}

static void SetScriptState (int script, DLevelScript::EScriptState state)
{
	DACSThinker *controller = DACSThinker::ActiveThinker;
	if (!controller)
		return;

	if (controller->RunningScripts[script])
		controller->RunningScripts[script]->SetState (state);
}

void P_DoDeferedScripts (void)
{
	acsdefered_t *def;
	int *scriptdata;
	AActor *gomo = NULL;

	// Handle defered scripts in this step, too
	def = level.info->defered;
	while (def)
	{
		acsdefered_t *next = def->next;
		switch (def->type)
		{
		case acsdefered_t::defexecute:
		case acsdefered_t::defexealways:
			scriptdata = level.behavior->FindScript (def->script);
			if (scriptdata)
			{
				if ((unsigned)def->playernum < MAXPLAYERS && idplayer(def->playernum).ingame())
					gomo = idplayer(def->playernum).mo;

				P_GetScriptGoing (gomo, NULL, def->script, scriptdata, 0, def->arg0, def->arg1, def->arg2, def->type == acsdefered_t::defexealways, true);

			} else
				Printf (PRINT_HIGH,"P_DoDeferredScripts: Unknown script %d\n", def->script);
			break;

		case acsdefered_t::defsuspend:
			SetScriptState (def->script, DLevelScript::SCRIPT_Suspended);
			DPrintf ("Defered suspend of script %d\n", def->script);
			break;

		case acsdefered_t::defterminate:
			SetScriptState (def->script, DLevelScript::SCRIPT_PleaseRemove);
			DPrintf ("Defered terminate of script %d\n", def->script);
			break;
		}
		delete def;
		def = next;
	}
	level.info->defered = NULL;
}

static void addDefered (level_pwad_info_t& i, acsdefered_t::EType type, int script, int arg0, int arg1, int arg2, AActor *who)
{
	if (i.levelnum != 0)
	{
		acsdefered_t *def = new acsdefered_s;

		def->next = i.defered;
		def->type = type;
		def->script = script;
		def->arg0 = arg0;
		def->arg1 = arg1;
		def->arg2 = arg2;
		if (who != NULL && who->player != NULL)
		{
			def->playernum = who->player->id;
		}
		else
		{
			def->playernum = -1;
		}
		i.defered = def;
		DPrintf ("Script %d on map %s defered\n", script, i.mapname.c_str());
	}
}

bool P_StartScript (AActor *who, line_t *where, int script, const char *map, int lineSide,
					int arg0, int arg1, int arg2, int always)
{
	if (level.mapname == map)
	{
		int *scriptdata;

		if (level.behavior != NULL &&
			(scriptdata = level.behavior->FindScript (script)) != NULL)
		{
			return P_GetScriptGoing (who, where, script,
									 scriptdata,
									 lineSide, arg0, arg1, arg2, always, false);
		}
		else
		{
			Printf (PRINT_HIGH,"P_StartScript: Unknown script %d\n", script);
		}
	}
	else
	{
		LevelInfos& levels = getLevelInfos();
		addDefered (levels.findByName(map),
					always ? acsdefered_t::defexealways : acsdefered_t::defexecute,
					script, arg0, arg1, arg2, who);
	}
	return false;
}

void P_SuspendScript (int script, const char *map)
{
	if (level.mapname != map)
	{
		LevelInfos& levels = getLevelInfos();
		addDefered(levels.findByName(map),
			acsdefered_t::defsuspend, script, 0, 0, 0, NULL);
	}
	else
	{
		SetScriptState(script, DLevelScript::SCRIPT_Suspended);
	}
}

void P_TerminateScript (int script, const char *map)
{
	if (level.mapname != map)
	{
		LevelInfos& levels = getLevelInfos();
		addDefered(levels.findByName(map),
			acsdefered_t::defterminate, script, 0, 0, 0, NULL);
	}
	else
	{
		SetScriptState (script, DLevelScript::SCRIPT_PleaseRemove);
	}
}

void strbin (char *str)
{
	char *p = str, c;
	int i;

	while ( (c = *p++) ) {
		if (c != '\\') {
			*str++ = c;
		} else {
			switch (*p) {
				case 'c':
					*str++ = '\034';	// TEXTCOLOR_ESCAPE
					break;
				case 'n':
					*str++ = '\n';
					break;
				case 't':
					*str++ = '\t';
					break;
				case 'r':
					*str++ = '\r';
					break;
				case '\n':
					break;
				case 'x':
				case 'X':
					c = 0;
					p++;
					for (i = 0; i < 2; i++) {
						c <<= 4;
						if (*p >= '0' && *p <= '9')
							c += *p-'0';
						else if (*p >= 'a' && *p <= 'f')
							c += 10 + *p-'a';
						else if (*p >= 'A' && *p <= 'F')
							c += 10 + *p-'A';
						else
							break;
						p++;
					}
					*str++ = c;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					c = 0;
					for (i = 0; i < 3; i++) {
						c <<= 3;
						if (*p >= '0' && *p <= '7')
							c += *p-'0';
						else
							break;
						p++;
					}
					*str++ = c;
					break;
				default:
					*str++ = *p;
					break;
			}
			p++;
		}
	}
	*str = 0;
}

FArchive &operator<< (FArchive &arc, acsdefered_s *defer)
{
	while (defer)
	{
		arc << (BYTE)1;
		arc << (BYTE)defer->type << defer->script
			<< defer->arg0 << defer->arg1 << defer->arg2;
		defer = defer->next;
	}
	arc << (BYTE)0;
	return arc;
}

FArchive &operator>> (FArchive &arc, acsdefered_s* &defertop)
{
	acsdefered_s **defer = &defertop;
	BYTE inbyte;

	arc >> inbyte;
	while (inbyte)
	{
		*defer = new acsdefered_s;
		arc >> inbyte;
		(*defer)->type = (acsdefered_s::EType)inbyte;
		arc >> (*defer)->script
			>> (*defer)->arg0 >> (*defer)->arg1 >> (*defer)->arg2;
		defer = &((*defer)->next);
		arc >> inbyte;
	}
	*defer = NULL;
	return arc;
}


BEGIN_COMMAND (scriptstat)
{
	if (DACSThinker::ActiveThinker == NULL)
	{
		Printf (PRINT_HIGH,"No scripts are running.\n");
	}
	else
	{
		DACSThinker::ActiveThinker->DumpScriptStatus ();
	}
}
END_COMMAND (scriptstat)

void DACSThinker::DumpScriptStatus ()
{
	static const char *stateNames[] =
	{
		"Running",
		"Suspended",
		"Delayed",
		"TagWait",
		"PolyWait",
		"ScriptWaitPre",
		"ScriptWait",
		"PleaseRemove"
	};
	DLevelScript *script = Scripts;

	while (script != NULL)
	{
		Printf (PRINT_HIGH,"%d: %s\n", script->script, stateNames[script->state]);
		script = script->next;
	}
}


VERSION_CONTROL (p_acs_cpp, "$Id$")
