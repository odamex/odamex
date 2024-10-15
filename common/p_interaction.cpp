// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


#include "odamex.h"

// Data.
#include "gstrings.h"
#include "m_random.h"
#include "i_system.h"
#include "c_console.h"
#include "p_local.h"
#include "s_sound.h"
#include "p_inter.h"
#include "p_lnspec.h"
#include "p_ctf.h"
#include "p_acs.h"
#include "g_gametype.h"
#include "m_wdlstats.h"
#include "svc_message.h"
#include "p_horde.h"
#include "com_misc.h"
#include "g_skill.h"
#include "p_mapformat.h"

#ifdef SERVER_APP
#include "sv_main.h"
#endif

extern bool predicting;

EXTERN_CVAR(sv_doubleammo)
EXTERN_CVAR(sv_weaponstay)
EXTERN_CVAR(sv_weapondamage)
EXTERN_CVAR(sv_monsterdamage)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_fragexitswitch) // [ML] 04/4/06: Added compromise for older exit method
EXTERN_CVAR(sv_friendlyfire)
EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_forcerespawn)
EXTERN_CVAR(sv_forcerespawntime)
EXTERN_CVAR(co_zdoomphys)
EXTERN_CVAR(cl_predictpickup)
EXTERN_CVAR(co_zdoomsound)
EXTERN_CVAR(co_globalsound)
EXTERN_CVAR(g_lives)

// sapientlion - experimental
EXTERN_CVAR(sv_weapondrop)

int MeansOfDeath;

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO] = {200, 50, 300, 50};
int clipammo[NUMAMMO] = {10, 4, 20, 1};

void AM_Stop(void);
void SV_SpawnMobj(AActor *mobj);
void SV_UpdateFrags(player_t &player);
void SV_CTFEvent(team_t f, flag_score_t event, player_t &who);
void SV_TouchSpecial(AActor *special, player_t *player);
ItemEquipVal SV_FlagTouch(player_t &player, team_t f, bool firstgrab);
void SV_SocketTouch(player_t &player, team_t f);
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void SV_SendDamagePlayer(player_t *player, AActor* inflictor, int healthDamage, int armorDamage);
void SV_SendDamageMobj(AActor *target, int pain);
void SV_ActorTarget(AActor *actor);
void PickupMessage(AActor *toucher, const char *message);
void WeaponPickupMessage(AActor *toucher, weapontype_t &Weapon);

#ifdef SERVER_APP
void SV_ShareKeys(card_t card, player_t& player);
#endif

static void PersistPlayerDamage(player_t& p)
{
	// Send this information to everybody.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		MSG_WriteSVC(&it->client.netbuf, SVC_PlayerMembers(p, SVC_PM_DAMAGE));
	}
}

static void PersistPlayerScore(player_t& p, const bool lives, const bool score)
{
	// Only run this on the server.
	if (!::serverside || ::clientside)
		return;

	// Don't bother if there's nothing we want to send.
	if (!(lives || score))
		return;

	// Either send flags, lives or both.
	unsigned flags = 0;
	if (lives)
		flags |= SVC_PM_LIVES;
	if (score)
		flags |= SVC_PM_SCORE;

	// Send this information to everybody.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		MSG_WriteSVC(&it->client.netbuf, SVC_PlayerMembers(p, flags));
	}
}

static void PersistTeamScore(team_t team)
{
	// Only run this on the server.
	if (!::serverside || ::clientside)
		return;

	// Send this information to everybody.
	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;
		MSG_WriteSVC(&it->client.netbuf, SVC_TeamMembers(team));
	}
}

//
// GET STUFF
//

// Give frags to a player
bool P_GiveFrags(player_t* player, int num)
{
	if (!G_CanScoreChange())
		return false;

	player->fragcount += num;

	if (G_IsRoundsGame() && !G_IsDuelGame() && !(sv_gametype == GM_CTF))
		player->totalpoints += num;

	return true;
}

// Give coop kills to a player
bool P_GiveKills(player_t* player, int num)
{
	if (!G_CanScoreChange())
		return false;

	player->killcount += num;
	return true;
}

// Give coop kills to a player
bool P_GiveDeaths(player_t* player, int num)
{
	if (!G_CanScoreChange())
		return false;

	player->deathcount += num;

	if (G_IsRoundsGame() && !G_IsDuelGame())
		player->totaldeaths += num;

	return true;
}

bool P_GiveMonsterDamage(player_t* player, int num)
{
	if (!G_CanScoreChange())
		return false;

	player->monsterdmgcount += num;
	return true;
}

// Give a specific number of points to a player's team
bool P_GiveTeamPoints(player_t* player, int num)
{
	if (!G_CanScoreChange())
		return false;

	GetTeamInfo(player->userinfo.team)->Points += num;
	return true;
}

/**
 * @brief Give lives to a player...or take them away.
 */
bool P_GiveLives(player_t* player, int num)
{
	if (!G_CanLivesChange())
		return false;

	player->lives += num;
	return true;
}

int P_GetFragCount(const player_t* player)
{
	if (G_IsRoundsGame() && !G_IsDuelGame())
		return player->totalpoints;

	return player->fragcount;
}

int P_GetPointCount(const player_t* player)
{
	if (G_IsRoundsGame())
		return player->totalpoints;

	return player->points;
}

int P_GetDeathCount(const player_t* player)
{
	if (G_IsRoundsGame())
		return player->totaldeaths;

	return player->deathcount;
}

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

// mbf21: take into account new weapon autoswitch flags
static ItemEquipVal P_GiveAmmoAutoSwitch(player_t* player, ammotype_t ammo, int oldammo)
{
	int i;

	// Keep the original behaviour while playbacking demos only.
	if (demoplayback)
	{
		switch (ammo)
		{
		case am_clip:
			if (player->readyweapon == wp_fist)
			{
				if (player->weaponowned[wp_chaingun])
					player->pendingweapon = wp_chaingun;
				else
					player->pendingweapon = wp_pistol;
			}
			break;

		case am_shell:
			if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
			{
				if (player->weaponowned[wp_shotgun])
					player->pendingweapon = wp_shotgun;
			}
			break;

		case am_cell:
			if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
				if (player->weaponowned[wp_plasma])
					player->pendingweapon = wp_plasma;
			break;

		case am_misl:
			if (player->readyweapon == wp_fist)
				if (player->weaponowned[wp_missile])
					player->pendingweapon = wp_missile;
		default:
			break;
		}
	}
	else if (player->userinfo.switchweapon != WPSW_NEVER)
	{
		if (weaponinfo[player->readyweapon].flags & WPF_AUTOSWITCHFROM &&
		    player->ammo[weaponinfo[player->readyweapon].ammotype] != ammo)
		{
			for (i = NUMWEAPONS - 1; i > player->readyweapon; --i)
			{
				if (player->weaponowned[i] &&
				    !(weaponinfo[i].flags & WPF_NOAUTOSWITCHTO) &&
				    weaponinfo[i].ammotype == ammo &&
				    weaponinfo[i].ammouse > oldammo &&
				    weaponinfo[i].ammouse <= player->ammo[ammo])
				{
					player->pendingweapon = (weapontype_t)i;
					break;
				}
			}
		}
	}
	return IEV_EquipRemove;
}

ItemEquipVal P_GiveAmmo(player_t *player, ammotype_t ammotype, float num)
{
	if (ammotype == am_noammo)
    {
		return IEV_NotEquipped;
    }

	if (ammotype < 0 || ammotype > NUMAMMO)
    {
		I_Error("P_GiveAmmo: bad type %i", ammotype);
    }

	if (player->ammo[ammotype] == player->maxammo[ammotype])
    {
		return IEV_NotEquipped;
    }

	if (num)
    {
		num *= clipammo[ammotype];
    }
	else
    {
		num = clipammo[ammotype] / 2;
    }

	if (sv_doubleammo)
	{
		// give double ammo in trainer mode,
		// you'll need in nightmare
		num *= G_GetCurrentSkill().double_ammo_factor;
	}
	else
	{
		num *= G_GetCurrentSkill().ammo_factor;
	}

	const int oldammotype = player->ammo[ammotype];
	player->ammo[ammotype] += static_cast<int>(num);

	if (player->ammo[ammotype] > player->maxammo[ammotype])
    {
		player->ammo[ammotype] = player->maxammo[ammotype];
    }

	// If non zero ammo,
	// don't change up weapons,
	// player was lower on purpose.
	if (oldammotype)
    {
		return IEV_EquipRemove;
    }

	// We were down to zero,
	// so select a new weapon.
	// Preferences are not user selectable.
	return P_GiveAmmoAutoSwitch(player, ammotype, oldammotype);

	//return IEV_EquipRemove;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
ItemEquipVal P_GiveWeapon(player_t *player, weapontype_t weapon, BOOL dropped)
{
	bool gaveammo;
	bool gaveweapon;

	// [RH] Don't get the weapon if no graphics for it
	state_t *state = states + weaponinfo[weapon].readystate;
	if ((state->frame & FF_FRAMEMASK) >= sprites[state->sprite].numframes)
	{
		return IEV_NotEquipped;
	}

	// [Toke - dmflags] old location of DF_WEAPONS_STAY
	if (multiplayer && sv_weaponstay && !dropped)
	{
		// leave placed weapons forever on net games
		if (player->weaponowned[weapon])
		{
			return IEV_NotEquipped;
		}

		player->bonuscount = BONUSADD;
		player->weaponowned[weapon] = true;

		if (!G_IsCoopGame())
		{
			P_GiveAmmo(player, weaponinfo[weapon].ammotype, 5);
		}
		else
		{
			P_GiveAmmo(player, weaponinfo[weapon].ammotype, 2);
		}

		if (P_CheckSwitchWeapon(player, weapon))
			player->pendingweapon = weapon;

		WeaponPickupMessage(player->mo, weapon);

		return IEV_EquipStay;
	}

	if (weaponinfo[weapon].ammotype != am_noammo)
	{
		// give one clip with a dropped weapon,
		// two clips with a found weapon
		if (dropped)
		{
			gaveammo = ((P_GiveAmmo(player, weaponinfo[weapon].ammotype, 1)) != 0);
		}
		else
		{
			gaveammo = ((P_GiveAmmo(player, weaponinfo[weapon].ammotype, 2)) != 0);
		}
	}
	else
	{
		gaveammo = false;
	}

	if (player->weaponowned[weapon])
	{
		gaveweapon = false;
	}
	else
	{
		gaveweapon = true;
		player->weaponowned[weapon] = true;
		if (P_CheckSwitchWeapon(player, weapon))
			player->pendingweapon = weapon;
	}

	if (gaveweapon || gaveammo)
		return IEV_EquipRemove;

	return IEV_NotEquipped;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
ItemEquipVal P_GiveBody(player_t *player, int num)
{
	if (player->health >= MAXHEALTH)
	{
		return IEV_NotEquipped;
	}

	player->health += static_cast<int>(static_cast<float>(num) * G_GetCurrentSkill().health_factor);
	if (player->health > MAXHEALTH)
	{
		player->health = MAXHEALTH;
	}
	player->mo->health = player->health;

	return IEV_EquipRemove;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
ItemEquipVal P_GiveArmor(player_t *player, int armortype)
{
	const int hits = armortype * 100;
	if (player->armorpoints >= hits)
	{
		return IEV_NotEquipped;	// don't pick up
	}

	const int hits_real = static_cast<int>(static_cast<float>(hits) * G_GetCurrentSkill().armor_factor);

	player->armortype = armortype;
	player->armorpoints += hits_real;
	if (player->armorpoints > hits)
		player->armorpoints = hits;

	return IEV_EquipRemove;
}

//
// P_GiveCard
//
ItemEquipVal P_GiveCard(player_t *player, card_t card)
{
	if (player->cards[card])
	{
		return IEV_NotEquipped;
	}

	player->bonuscount = BONUSADD;
	player->cards[card] = 1;

	if (multiplayer)
	{
#ifdef SERVER_APP
		// Register the key
		SV_ShareKeys(card, *player);	
#endif


		return IEV_EquipStay;
	}

	return IEV_EquipRemove;
}

//
// P_GivePower
//
ItemEquipVal P_GivePower(player_t *player, int /*powertype_t*/ power)
{
	if (power == pw_invulnerability)
	{
		player->powers[power] = INVULNTICS;
		return IEV_EquipRemove;
	}

	if (power == pw_invisibility)
	{
		player->powers[power] = INVISTICS;
		player->mo->flags |= MF_SHADOW;
		return IEV_EquipRemove;
	}

	if (power == pw_infrared)
	{
		player->powers[power] = INFRATICS;
		return IEV_EquipRemove;
	}

	if (power == pw_ironfeet)
	{
		player->powers[power] = IRONTICS;
		return IEV_EquipRemove;
	}

	if (power == pw_strength)
	{
		P_GiveBody(player, 100);
		player->powers[power] = 1;
		return IEV_EquipRemove;
	}

	if (player->powers[power])
	{
		return IEV_NotEquipped;	// already got it
	}

	player->powers[power] = 1;
	return IEV_EquipRemove;
}

#include "v_textcolors.h"

/**
 * @brief Give the player a care package.
 * 
 * @detail A care package gives you a small collection of items based on what
 *         you're already holding.  TODO: These messages should be LANGUAGE'ed.
 */
static void P_GiveCarePack(player_t* player)
{
	const int ammomulti[NUMAMMO] = {2, 1, 1, 2};

	// [AM] There is way too much going on in here to accurately predict.
	if (!::serverside)
		return;

	// Which weapons will we need ammo for?
	bool hasWeap[NUMAMMO] = {false, false, false, false};
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		const ammotype_t ammo = ::weaponinfo[i].ammotype;
		if (ammo == am_noammo)
		{
			continue;
		}
		if (player->weaponowned[i])
		{
			hasWeap[ammo] = true;
		}
	}

	// We get "blocks" of inventory to give out.
	int blocks = 4;
	std::string message, midmessage;

	// Players who are extremely low on health always get an initial health
	// boost.
	if (blocks >= 1 && player->mo->health < 25)
	{
		P_GiveBody(player, 25);
		blocks -= 1;
	}

	// Players who are extremely low on ammo for a weapon they are holding
	// always get ammo for that weapon.
	const hordeDefine_t::ammos_t& ammos = P_HordeAmmos();
	for (size_t i = 0; i < ammos.size(); i++)
	{
		const ammotype_t ammo = ammos.at(i);
		if (blocks < 1)
		{
			break;
		}

		// Don't stockpile ammo for weapons we don't have.
		if (!hasWeap[ammo])
		{
			continue;
		}

		// Missle clip is a bit stingy, so we double our handouts.
		const int lowLimit = ammomulti[ammo] * 2;
		const float giveAmount = static_cast<float>(ammomulti[ammo] * 5);

		if (player->ammo[ammo] < ::clipammo[ammo] * lowLimit)
		{
			P_GiveAmmo(player, ammo, giveAmount);
			blocks -= 1;
		}
	}

	// Players who have less than 100 health at this point get another health pack.
	if (blocks >= 1 && player->mo->health < 100)
	{
		P_GiveBody(player, 25);
		blocks -= 1;
	}

	// Give the player a missing weapon - just one.
	if (blocks >= 1)
	{
		const hordeDefine_t::weapons_t& weapons = P_HordeWeapons();
		for (size_t i = 0; i < weapons.size(); i++)
		{
			// No weapon is a special case that means give the player
			// berserk strength (without the health).
			if (weapons.at(i) == wp_none && player->powers[pw_strength] < 1)
			{
				player->powers[pw_strength] = 1;
				blocks -= 1;

				message = "You found a berserk stim in this supply cache!";
				midmessage = "Got berserk";
				break;
			}
			else if (weapons.at(i) != wp_none && !player->weaponowned[weapons.at(i)])
			{
				P_GiveWeapon(player, weapons.at(i), false);
				blocks -= 1;

				message = "You found a weapon in this supply cache!";
				switch (weapons.at(i))
				{
				case wp_chainsaw:
					midmessage = "Got Chainsaw";
					break;
				case wp_pistol:
					midmessage = "Got Pistol";
					break;
				case wp_shotgun:
					midmessage = "Got Shotgun";
					break;
				case wp_chaingun:
					midmessage = "Got Chaingun";
					break;
				case wp_missile:
					midmessage = "Got Rocket Launcher";
					// [AM] Default missile clip is stingy.
					P_GiveAmmo(player, am_misl, 2);
					break;
				case wp_plasma:
					midmessage = "Got Plasmagun";
					break;
				case wp_bfg:
					midmessage = "Got BFG9000";
					break;
				case wp_supershotgun:
					midmessage = "Got Super Shotgun";
					break;
				case wp_none:
				case wp_fist:
				case wp_nochange:
				case NUMWEAPONS:
					break;
				}
				
				break;
			}
		}
	}

	// Hand out some ammo for all held weapons - that's always appreciated.
	// If there are fewer than four ammos, we hand out more for the ones
	// we have.
	if (blocks >= 1 && !ammos.empty())
	{
		for (size_t i = 0; i < 4; i++)
		{
			const ammotype_t ammo = ammos.at(i % ammos.size());

			// Don't stockpile ammo for weapons we don't have.
			if (!hasWeap[ammo])
			{
				continue;
			}

			P_GiveAmmo(player, ammo, ammomulti[ammo]);
		}
		blocks -= 1;
	}

	// We got this far, why not top off players armor?
	if (blocks >= 1 && player->armorpoints + 10 < 95)
	{
		player->armorpoints += 10;
		if (player->armorpoints > ::deh.MaxArmor)
		{
			player->armorpoints = ::deh.MaxArmor;
		}
		if (!player->armortype)
		{
			player->armortype = ::deh.GreenAC;
		}

		blocks -= 1;
	}

	if (message.empty())
		message = "Picked up a supply cache full of health and ammo!";

	if (!::clientside)
	{
		// [AM] FIXME: This gives players their inventory, with no
		//             background flash.
		MSG_WriteSVC(&player->client.reliablebuf, SVC_PlayerInfo(*player));
		MSG_WriteSVC(&player->client.reliablebuf, SVC_Print(PRINT_PICKUP, message + "\n"));
		if (!midmessage.empty())
		{
			std::string buf = std::string(TEXTCOLOR_GREEN) + midmessage;
			MSG_WriteSVC(&player->client.reliablebuf, SVC_MidPrint(buf, 0));
		}
	}
	else
	{
		Printf(PRINT_PICKUP, "%s\n", message.c_str());
		if (!midmessage.empty())
		{
			std::string buf = std::string(TEXTCOLOR_GREEN) + midmessage;
			C_MidPrint(buf.c_str(), NULL, 0);
		}
	}
}

bool P_SpecialIsWeapon(AActor *special)
{
	if (!special)
		return false;

	return (special->type == MT_CHAINGUN ||
			special->type == MT_SHOTGUN  ||
			special->type == MT_SUPERSHOTGUN ||
			special->type == MT_MISC25 ||
			special->type == MT_MISC26 ||
			special->type == MT_MISC27 ||
			special->type == MT_MISC28);
}

void P_PickupSound(AActor *ent, int channel, const char *name)
{
	if (serverside && co_globalsound) //Send pickup sound to all other players
		UV_SoundAvoidPlayer(ent, channel, name, co_zdoomsound ? ATTN_NORM : ATTN_NONE);
	else if (clientside && ent == consoleplayer().mo) //Only play our own pickup sounds, the server will send other players pickup sounds if needed
		S_Sound(ent, channel, name, 1, ATTN_NONE);
}

void P_GiveSpecial(player_t *player, AActor *special)
{
	if (!player || !player->mo || !special)
		return;

	AActor *toucher = player->mo;
	int sound = 0;
	const OString* msg = NULL;
	ItemEquipVal val = IEV_EquipRemove;
	bool dropped = false;

	// Identify by sprite.
	switch (special->sprite)
	{
		// armor
	    case SPR_ARM1:
			val = P_GiveArmor(player, deh.GreenAC);
			msg = &GOTARMOR;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_GREENARMOR, false);
		    }
            break;

	    case SPR_ARM2:
			val = P_GiveArmor(player, deh.BlueAC);
			msg = &GOTMEGA;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_BLUEARMOR, false);
		    }
            break;

		// bonus items
	    case SPR_BON1:
            player->health += static_cast<int>(G_GetCurrentSkill().health_factor); // can go over 100%
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
			msg = &GOTHTHBONUS;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_HEALTHBONUS, false);
            break;

	    case SPR_BON2:
            player->armorpoints += static_cast<int>(G_GetCurrentSkill().armor_factor); // can go over 100%
            if (player->armorpoints > deh.MaxArmor)
            {
                player->armorpoints = deh.MaxArmor;
            }
            if (!player->armortype)
            {
                player->armortype = deh.GreenAC;
            }
			msg = &GOTARMBONUS;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_ARMORBONUS, false);
            break;

	    case SPR_SOUL:
            player->health += static_cast<int>(static_cast<float>(deh.SoulsphereHealth) * G_GetCurrentSkill().health_factor);
            if (player->health > deh.MaxSoulsphere)
            {
                player->health = deh.MaxSoulsphere;
            }
            player->mo->health = player->health;
			msg = &GOTSUPER;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_SOULSPHERE, false);
            break;

	    case SPR_MEGA:
            player->health = static_cast<int>(static_cast<float>(deh.MegasphereHealth) * G_GetCurrentSkill().health_factor);
            player->mo->health = player->health;
            P_GiveArmor(player,deh.BlueAC);
			msg = &GOTMSPHERE;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_MEGASPHERE, false);
            break;

		// cards
	    case SPR_BKEY:
			val = P_GiveCard(player, it_bluecard);
			msg = &GOTBLUECARD;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_BLUEKEY, false);
		    }
            break;

	    case SPR_YKEY:
            val = P_GiveCard(player, it_yellowcard);
			msg = &GOTYELWCARD;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_YELLOWKEY, false);
		    }
            break;

	    case SPR_RKEY:
            val = P_GiveCard(player, it_redcard);
			msg = &GOTREDCARD;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_REDKEY, false);
		    }
            break;

	    case SPR_BSKU:
            val = P_GiveCard(player, it_blueskull);
			msg = &GOTBLUESKUL;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_BLUESKULL, false);
		    }
            break;

	    case SPR_YSKU:
            val = P_GiveCard(player, it_yellowskull);
			msg = &GOTYELWSKUL;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_YELLOWSKULL, false);
		    }
            break;

	    case SPR_RSKU:
            val = P_GiveCard(player, it_redskull);
			msg = &GOTREDSKUL;
            sound = 3;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_REDSKULL, false);
		    }
            break;

		// medikits, heals
	    case SPR_STIM:
			val = P_GiveBody(player, 10);
			msg = &GOTSTIM;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_STIMPACK, false);
		    }
            break;

	    case SPR_MEDI:
            if (player->health < 25)
            {
				msg = &GOTMEDINEED;
            }
            else if (player->health < 100)
            {
                msg = &GOTMEDIKIT;
            }
			val = P_GiveBody(player, 25);
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_MEDKIT, false);
		    }
            break;

		// power ups
	    case SPR_PINV:
            val = P_GivePower(player, pw_invulnerability);
			msg = &GOTINVUL;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_INVULNSPHERE, false);
            break;

	    case SPR_PSTR:
			val = P_GivePower(player, pw_strength);
			msg = &GOTBERSERK;
            if (player->readyweapon != wp_fist)
            {
                player->pendingweapon = wp_fist;
            }
            sound = 1;
			M_LogWDLPickupEvent(player, special, WDL_PICKUP_BERSERK, false);
            break;

	    case SPR_PINS:
            val = P_GivePower(player, pw_invisibility);
			msg = &GOTINVIS;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_INVISSPHERE, false);
            break;

	    case SPR_SUIT:
            val = P_GivePower(player, pw_ironfeet);
			msg = &GOTSUIT;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_RADSUIT, false);
            break;

	    case SPR_PMAP:
			val = P_GivePower(player, pw_allmap);
			msg = &GOTMAP;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_COMPUTERMAP, false);
            break;

	    case SPR_PVIS:
            val = P_GivePower(player, pw_infrared);
			msg = &GOTVISOR;
            sound = 1;
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_GOGGLES, false);
            break;

		// ammo
	    case SPR_CLIP:
            if (special->flags & MF_DROPPED)
            {
				val = P_GiveAmmo(player, am_clip, 0);
			    dropped = true;
            }
            else
            {
				val = P_GiveAmmo(player, am_clip, 1);
            }
			msg = &GOTCLIP;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CLIP, dropped);
		    }
            break;

	    case SPR_AMMO:
			val = P_GiveAmmo(player, am_clip, 5);
			msg = &GOTCLIPBOX;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_AMMOBOX, false);
		    }
            break;

	    case SPR_ROCK:
            val = P_GiveAmmo(player, am_misl, 1);
			msg = &GOTROCKET;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_ROCKET, false);
		    }
            break;

	    case SPR_BROK:
            val = P_GiveAmmo(player, am_misl, 5);
			msg = &GOTROCKBOX;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_ROCKETBOX, false);
		    }
            break;

	    case SPR_CELL:
            val = P_GiveAmmo(player, am_cell, 1);
			msg = &GOTCELL;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CELL, false);
		    }
            break;

	    case SPR_CELP:
            val = P_GiveAmmo(player, am_cell, 5);
			msg = &GOTCELLBOX;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CELLPACK, false);
		    }
            break;

	    case SPR_SHEL:
		    if (special->flags & MF_DROPPED)
		    {
			    dropped = true;
		    }
		    val = P_GiveAmmo(player, am_shell, 1);
		    msg = &GOTSHELLS;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_SHELLS, dropped);
		    }
            break;

	    case SPR_SBOX:
			val = P_GiveAmmo(player, am_shell, 5);
			msg = &GOTSHELLBOX;
		    if (val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_SHELLBOX, false);
		    }
            break;

	    case SPR_BPAK:
            if (!player->backpack)
            {
                for (int i=0 ; i<NUMAMMO ; i++)
                {
                    player->maxammo[i] *= 2;
                }
                player->backpack = true;
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_BACKPACK, false);
            }
            for (int i=0 ; i<NUMAMMO ; i++)
            {
                P_GiveAmmo(player, (ammotype_t)i, 1);
            }
			msg = &GOTBACKPACK;
			break;

		case SPR_CARE:
			// Care package.  What does it contian?
			P_GiveCarePack(player);
		    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CAREPACKAGE, false);
			break;

		// weapons
	    case SPR_BFUG:
            val = P_GiveWeapon(player, wp_bfg, special->flags & MF_DROPPED);
			msg = &GOTBFG9000;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_BFG,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_MGUN:
            val = P_GiveWeapon(player, wp_chaingun, special->flags & MF_DROPPED);
			msg = &GOTCHAINGUN;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CHAINGUN,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_CSAW:
			val = P_GiveWeapon(player, wp_chainsaw, special->flags & MF_DROPPED);
			msg = &GOTCHAINSAW;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_CHAINSAW,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_LAUN:
            val = P_GiveWeapon(player, wp_missile, special->flags & MF_DROPPED);
			msg = &GOTLAUNCHER;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_ROCKETLAUNCHER,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_PLAS:
			val = P_GiveWeapon(player, wp_plasma, special->flags & MF_DROPPED);
			msg = &GOTPLASMA;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_PLASMAGUN,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_SHOT:
            val = P_GiveWeapon(player, wp_shotgun, special->flags & MF_DROPPED);
			msg = &GOTSHOTGUN;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_SHOTGUN,
			                        special->flags & MF_DROPPED);
		    }
            break;

	    case SPR_SGN2:
			val = P_GiveWeapon(player, wp_supershotgun, special->flags & MF_DROPPED);
			msg = &GOTSHOTGUN2;
            sound = 2;
		    if (val == IEV_EquipStay || val == IEV_EquipRemove)
		    {
			    M_LogWDLPickupEvent(player, special, WDL_PICKUP_SUPERSHOTGUN,
			                        special->flags & MF_DROPPED);
		    }
            break;

        default:
		{
			bool teamItemSuccess = false;
			for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
			{
				TeamInfo* teamInfo = GetTeamInfo((team_t)iTeam);

				if (teamInfo->FlagSprite == special->sprite || teamInfo->FlagDownSprite == special->sprite)
				{
					val = SV_FlagTouch(*player, teamInfo->Team, teamInfo->FlagSprite == special->sprite);
					sound = -1;
					teamItemSuccess = true;
					break;
				}

				if (teamInfo->FlagSocketSprite == special->sprite)
				{
					SV_SocketTouch(*player, teamInfo->Team);
					return;
				}
			}

			if (!teamItemSuccess)
			{
				Printf(PRINT_HIGH, "P_SpecialThing: Unknown gettable thing %d: %s\n", special->sprite, special->info->name);
				return;
			}
		}
	}

	if (special->flags & MF_COUNTITEM)
	{
		player->itemcount++;
		level.found_items++;
	}

	if (val == IEV_NotEquipped)
		return;

	//the player equipped/picked up an item
	player->bonuscount = BONUSADD;
	SV_TouchSpecial(special, player);

	if (msg != NULL)
		PickupMessage(toucher, GStrings(*msg));

	if (val == IEV_EquipRemove)
		special->Destroy();

	AActor *ent = player->mo;
	switch (sound)
	{
	case 0:
	case 3:
		P_PickupSound(ent, CHAN_ITEM, "misc/i_pkup");
		break;
	case 1:
		P_PickupSound(ent, CHAN_ITEM, "misc/p_pkup");
		break;
	case 2:
		P_PickupSound(ent, CHAN_ITEM, "misc/w_pkup");
		break;
	}
}


//
// P_TouchSpecialThing
//
void P_TouchSpecialThing(AActor *special, AActor *toucher)
{
	// Somebody passed null pointers. [Toke - fix99]
	if (!toucher || !special)
		return;

	// Spectators shouldn't be able to touch things.
	if (toucher->player && toucher->player->spectator)
		return;

	// Touchers that aren't players or avatars need not apply.
	if (!toucher->player && toucher->type != MT_AVATAR)
		return;

	if (predicting)
		return;

	// Dead thing touching.
	// Can happen with a sliding player corpse.
	if (toucher->health <= 0)
		return;

	// out of reach?
	fixed_t delta = special->z - toucher->z;
	fixed_t lowerbound = co_zdoomphys ? -32*FRACUNIT : -8*FRACUNIT;

	if (delta > toucher->height || delta < lowerbound)
		return;

	// Only allow clients to predict touching weapons, not health, armor, etc
	if (!serverside && (!cl_predictpickup || !P_SpecialIsWeapon(special)))
		return;

	// [Blair] Execute ZDoom thing specials on items that are picked up.
	// (Then remove the special.)
	if (special->special)
	{
		LineSpecials[special->special](NULL, toucher, special->args[0], special->args[1],
		                               special->args[2], special->args[3],
		                               special->args[4]);
		special->special = 0;
	}

	if (toucher->type == MT_AVATAR)
	{
		PlayersView pr = PlayerQuery().execute().players;
		for (PlayersView::iterator it = pr.begin(); it != pr.end(); ++it)
		{
			P_GiveSpecial(*it, special);
		}
	}
	else if (toucher->player)
	{
		P_GiveSpecial(toucher->player, special);
	}
}


// [RH]
// SexMessage: Replace parts of strings with gender-specific pronouns
//
// The following expansions are performed:
//		%g -> he/she/it
//		%h -> him/her/it
//		%p -> his/her/its
//		%o -> other (victim)
//		%k -> killer
//
void SexMessage (const char *from, char *to, int gender, const char *victim, const char *killer)
{
	static const char *genderstuff[3][3] =
	{
		{ "he",  "him", "his" },
		{ "she", "her", "her" },
		{ "it",  "it",  "its" }
	};
	static const int gendershift[3][3] =
	{
		{ 2, 3, 3 },
		{ 3, 3, 3 },
		{ 2, 2, 3 }
	};
	const char *subst = NULL;

	do
	{
		if (*from != '%')
		{
			*to++ = *from;
		}
		else
		{
			int gendermsg = -1;

			switch (from[1])
			{
			case 'g':	gendermsg = 0;	break;
			case 'h':	gendermsg = 1;	break;
			case 'p':	gendermsg = 2;	break;
			case 'o':	subst = victim;	break;
			case 'k':	subst = killer;	break;
			}
			if (subst != NULL)
			{
				int len = strlen (subst);
				memcpy (to, subst, len);
				to += len;
				from++;
				subst = NULL;
			}
			else if (gendermsg < 0)
			{
				*to++ = '%';
			}
			else
			{
				strcpy (to, genderstuff[gender][gendermsg]);
				to += gendershift[gender][gendermsg];
				from++;
			}
		}
	} while (*from++);
}

//
// [RH]
// ClientObituary: Show a message when a player dies
//
static void ClientObituary(AActor* self, AActor* inflictor, AActor* attacker)
{
	char gendermessage[1024];

	if (!self || !self->player)
		return;

	// Don't print obituaries after the end of a round
	if (!G_CanShowObituary() || gamestate != GS_LEVEL)
		return;

	int gender = self->player->userinfo.gender;

	// Treat voodoo dolls as unknown deaths
	if (inflictor && inflictor->player == self->player)
		MeansOfDeath = MOD_UNKNOWN;

	if (G_IsCoopGame())
		MeansOfDeath |= MOD_FRIENDLY_FIRE;

	if (G_IsTeamGame() && attacker && attacker->player &&
	    self->player->userinfo.team == attacker->player->userinfo.team)
		MeansOfDeath |= MOD_FRIENDLY_FIRE;

	bool friendly = MeansOfDeath & MOD_FRIENDLY_FIRE;
	int mod = MeansOfDeath & ~MOD_FRIENDLY_FIRE;
	const char* message = NULL;
	OString messagename;

	switch (mod)
	{
	case MOD_SUICIDE:
		messagename = OB_SUICIDE;
		break;
	case MOD_FALLING:
		messagename = OB_FALLING;
		break;
	case MOD_CRUSH:
		messagename = OB_CRUSH;
		break;
	case MOD_EXIT:
		messagename = OB_EXIT;
		break;
	case MOD_WATER:
		messagename = OB_WATER;
		break;
	case MOD_SLIME:
		messagename = OB_SLIME;
		break;
	case MOD_LAVA:
		messagename = OB_LAVA;
		break;
	case MOD_BARREL:
		messagename = OB_BARREL;
		break;
	case MOD_SPLASH:
		messagename = OB_SPLASH;
		break;
	}

	if (!messagename.empty())
		message = GStrings(messagename);

	if (attacker && message == NULL)
	{
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_R_SPLASH:
				messagename = OB_R_SPLASH;
				break;
			case MOD_ROCKET:
				messagename = OB_ROCKET;
				break;
			default:
				messagename = OB_KILLEDSELF;
				break;
			}
			message = GStrings(messagename);
		}
		else if (!attacker->player)
		{
			if (mod == MOD_HIT)
			{
				switch (attacker->type)
				{
				case MT_UNDEAD:
					messagename = OB_UNDEADHIT;
					break;
				case MT_TROOP:
					messagename = OB_IMPHIT;
					break;
				case MT_HEAD:
					messagename = OB_CACOHIT;
					break;
				case MT_SERGEANT:
					messagename = OB_DEMONHIT;
					break;
				case MT_SHADOWS:
					messagename = OB_SPECTREHIT;
					break;
				case MT_BRUISER:
					messagename = OB_BARONHIT;
					break;
				case MT_KNIGHT:
					messagename = OB_KNIGHTHIT;
					break;
				case MT_SKULL:
					// [AM] Lost soul attacks now damage using MOD_HIT.
					messagename = OB_SKULL;
					break;
				default:
					messagename = OB_GENMONHIT;
					break;
				}
			}
			else
			{
				switch (attacker->type)
				{
				case MT_POSSESSED:
					messagename = OB_ZOMBIE;
					break;
				case MT_SHOTGUY:
					messagename = OB_SHOTGUY;
					break;
				case MT_VILE:
					messagename = OB_VILE;
					break;
				case MT_UNDEAD:
					messagename = OB_UNDEAD;
					break;
				case MT_FATSO:
					messagename = OB_FATSO;
					break;
				case MT_CHAINGUY:
					messagename = OB_CHAINGUY;
					break;
				case MT_TROOP:
					messagename = OB_IMP;
					break;
				case MT_HEAD:
					messagename = OB_CACO;
					break;
				case MT_BRUISER:
					messagename = OB_BARON;
					break;
				case MT_KNIGHT:
					messagename = OB_KNIGHT;
					break;
				case MT_SPIDER:
					messagename = OB_SPIDER;
					break;
				case MT_BABY:
					messagename = OB_BABY;
					break;
				case MT_CYBORG:
					messagename = OB_CYBORG;
					break;
				case MT_WOLFSS:
					messagename = OB_WOLFSS;
					break;
				default:
					if (mod == MOD_HITSCAN)
					{
						messagename = OB_GENMONPEW;
					}
					else if (mod == MOD_ROCKET || mod == MOD_R_SPLASH)
					{
						messagename = OB_GENMONBOOM;
					}
					else
					{
						messagename = OB_GENMONPROJ;
					}
					break;
				}
			}

			if (!messagename.empty())
				message = GStrings(messagename);
		}
	}

	if (message)
	{
		SexMessage(message, gendermessage, gender, self->player->userinfo.netname.c_str(),
		           self->player->userinfo.netname.c_str());
		SV_BroadcastPrintf(PRINT_OBITUARY, "%s\n", gendermessage);

		toast_t toast;
		toast.flags = toast_t::ICON | toast_t::RIGHT_PID;
		toast.icon = mod;
		toast.right_pid = self->player->id;
		COM_PushToast(toast);
		return;
	}

	if (attacker && attacker->player)
	{
		if (friendly)
		{
			gender = attacker->player->userinfo.gender;
			messagename =
			    GStrings.getIndex(GStrings.toIndex(OB_FRIENDLY1) + (P_Random() & 3));
			message = messagename.c_str();
		}
		else
		{
			switch (mod)
			{
			case MOD_FIST:
				messagename = OB_MPFIST;
				break;
			case MOD_CHAINSAW:
				messagename = OB_MPCHAINSAW;
				break;
			case MOD_PISTOL:
				messagename = OB_MPPISTOL;
				break;
			case MOD_SHOTGUN:
				messagename = OB_MPSHOTGUN;
				break;
			case MOD_SSHOTGUN:
				messagename = OB_MPSSHOTGUN;
				break;
			case MOD_CHAINGUN:
				messagename = OB_MPCHAINGUN;
				break;
			case MOD_ROCKET:
				messagename = OB_MPROCKET;
				break;
			case MOD_R_SPLASH:
				messagename = OB_MPR_SPLASH;
				break;
			case MOD_PLASMARIFLE:
				messagename = OB_MPPLASMARIFLE;
				break;
			case MOD_BFG_BOOM:
				messagename = OB_MPBFG_BOOM;
				break;
			case MOD_BFG_SPLASH:
				messagename = OB_MPBFG_SPLASH;
				break;
			case MOD_TELEFRAG:
				messagename = OB_MPTELEFRAG;
				break;
			case MOD_RAILGUN:
				messagename = OB_RAILGUN;
				break;
			default:
				messagename = OB_KILLED; // If someone was killed by someone, show it
				break;
			}

			if (!messagename.empty())
				message = GStrings(messagename);
		}
	}

	if (message && attacker && attacker->player)
	{
		SexMessage(message, gendermessage, gender, self->player->userinfo.netname.c_str(),
		           attacker->player->userinfo.netname.c_str());
		SV_BroadcastPrintf(PRINT_OBITUARY, "%s\n", gendermessage);

		toast_t toast;
		toast.flags = toast_t::LEFT_PID | toast_t::ICON | toast_t::RIGHT_PID;
		toast.left_pid = attacker->player->id;
		toast.icon = mod;
		toast.right_pid = self->player->id;
		COM_PushToast(toast);
		return;
	}

	SexMessage(GStrings(OB_DEFAULT), gendermessage, gender,
	           self->player->userinfo.netname.c_str(),
	           self->player->userinfo.netname.c_str());
	SV_BroadcastPrintf(PRINT_OBITUARY, "%s\n", gendermessage);

	toast_t toast;
	toast.flags = toast_t::ICON | toast_t::RIGHT_PID;
	toast.icon = mod;
	toast.right_pid = self->player->id;
	COM_PushToast(toast);
}

//
// P_KillMobj
//
void P_KillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill)
{
	SV_SendKillMobj(source, target, inflictor, joinkill);
	AActor *mo;
	player_t *splayer;
	player_t *tplayer;

	bool sendScore = false;
	bool sendLives = false;
	bool sendTeamScore = false;

	target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

	// GhostlyDeath -- Joinkill is only set on players, so we should be safe!
	if (joinkill)
	{
		target->oflags |= MFO_SPECTATOR;
		target->flags2 |= MF2_FLY;
	}

	if (target->type != MT_SKULL)
    {
		target->flags &= ~MF_NOGRAVITY;
    }

	target->flags |= MF_CORPSE|MF_DROPOFF;
	target->height >>= 2;

	// [RH] If the thing has a special, execute and remove it
	//		Note that the thing that killed it is considered
	//		the activator of the script.
	if ((target->flags & MF_COUNTKILL) && target->special)
	{
		LineSpecials[target->special] (NULL, source, target->args[0],
									   target->args[1], target->args[2],
									   target->args[3], target->args[4]);
		target->special = 0;
	}
	// [RH] Also set the thing's tid to 0. [why?]
	target->tid = 0;

	if (serverside && target->flags & MF_COUNTKILL)
		level.killed_monsters++;

	if (source)
	{
		splayer = source->player;
	}
	else
	{
		splayer = 0;
	}

	tplayer = target->player;

	if (tplayer)
	{
		// [SL] 2011-06-26 - Set the player's attacker.  For some reason this
		// was not being set clientside
		tplayer->attacker = source ? source->ptr() : AActor::AActorPtr();
	}

	if (source && source->player)
	{
		// Don't count any frags at level start, because they're just telefrags
		// resulting from insufficient deathmatch starts, and it wouldn't be
		// fair to count them toward a player's score.
		if (target->player && level.time)
		{
			if (!joinkill)
			{
				if (target->player == source->player) // [RH] Cumulative frag count
				{
					sendScore |= P_GiveFrags(splayer, -1);
					// [Toke] Minus a team frag for suicide
					if (sv_gametype == GM_TEAMDM)
					{
						sendTeamScore |= P_GiveTeamPoints(splayer, -1);
					}
				}
				// [Toke] Minus a team frag for killing teammate
				else if (G_IsTeamGame() &&
				         (splayer->userinfo.team == tplayer->userinfo.team))
				{
					// [Toke - Teamplay || deathz0r - updated]
					sendScore |= P_GiveFrags(splayer, -1);
					if (sv_gametype == GM_TEAMDM)
					{
						sendTeamScore |= P_GiveTeamPoints(splayer, -1);
					}
					else if (sv_gametype == GM_CTF)
					{
						SV_CTFEvent((team_t)0, SCORE_BETRAYAL, *splayer);
					}
				}
				else
				{
					sendScore |= P_GiveFrags(splayer, 1);
					// [Toke] Add a team frag
					if (sv_gametype == GM_TEAMDM)
					{
						sendTeamScore |= P_GiveTeamPoints(splayer, 1);
					}
					else if (sv_gametype == GM_CTF)
					{
						if (tplayer->flags[splayer->userinfo.team])
						{
							SV_CTFEvent((team_t)0, SCORE_CARRIERKILL, *splayer);
						}
						else
						{
							SV_CTFEvent((team_t)0, SCORE_KILL, *splayer);
						}
					}
				}
			}

			if (sendLives || sendScore)
				PersistPlayerScore(*splayer, sendLives, sendScore);
			if (sendTeamScore)
				PersistTeamScore(splayer->userinfo.team);
		}
		// [deathz0r] Stats for co-op scoreboard
		if (G_IsCoopGame() &&
		    ((target->flags & MF_COUNTKILL) || (target->type == MT_SKULL)))
		{
			if (P_GiveKills(splayer, 1))
				PersistPlayerScore(*splayer, sendLives, sendScore);
		}
	}

	sendScore = sendLives = sendTeamScore = false;

	if (target->player)
	{
		// [Toke - CTF]
		if (sv_gametype == GM_CTF)
			CTF_CheckFlags(*target->player);

		if (!joinkill)
			sendScore |= P_GiveDeaths(tplayer, 1);

		if (!joinkill && tplayer->lives > 0)
			sendLives |= P_GiveLives(tplayer, -1);

		// Death script execution, care of Skull Tag
		if (level.behavior != NULL)
		{
			level.behavior->StartTypedScripts (SCRIPT_Death, target);
		}

		// count environment kills against you
		if (!source && !joinkill)
		{
			// [RH] Cumulative frag count
			sendScore |= P_GiveFrags(tplayer, -1);
		}

		// [NightFang] - Added this, thought it would be cooler
		// [Fly] - No, it's not cooler
		// target->player->cheats = CF_CHASECAM;

		if (sendLives || sendScore)
			PersistPlayerScore(*tplayer, sendLives, sendScore);

		target->flags &= ~MF_SOLID;
		target->player->playerstate = PST_DEAD;
		P_DropWeapon(target->player);

		tplayer->suicidedelay = SuicideDelay;
		tplayer->death_time = level.time;

		if (target == consoleplayer().camera)
		{
			// don't die in auto map, switch view prior to dying
			AM_Stop();
		}
	}

	if (target->health > 0) // denis - when this function is used standalone
	{
		target->health = 0;
	}

	P_RemoveHealthPool(target);
	P_QueueCorpseForDestroy(target);

    if (target->info->xdeathstate && target->health < target->info->gibhealth)
    {
        P_SetMobjState(target, target->info->xdeathstate);
    }
    else
    {
        P_SetMobjState(target, target->info->deathstate);
    }

	target->tics -= P_Random(target) & 3;

	if (target->tics < 1)
	{
		target->tics = 1;
	}

	// [RH] Death messages
	// Nes - Server now broadcasts obituaries.
	// [CG] Since this is a stub, no worries anymore.
	if (target->player && ::level.time && !::clientside && !::demoplayback && !joinkill)
	{
		ClientObituary(target, inflictor, source);
	}

	// [AM] Save the "out of lives" message until after the obit.
	if (g_lives && tplayer && tplayer->lives <= 0)
		SV_BroadcastPrintf("%s is out of lives.\n",
		                   tplayer->userinfo.netname.c_str());

	// Check sv_fraglimit.
	if (source && source->player && target->player && level.time)
	{
		// [Toke] Better sv_fraglimit
		if (sv_gametype == GM_DM)
			G_FragsCheckEndGame();

		// [Toke] TeamDM sv_fraglimit
		if (sv_gametype == GM_TEAMDM)
			G_TeamFragsCheckEndGame();
	}

	// Check survival endgame
	if (target->player && level.time)
		G_LivesCheckEndGame();

	if (gamemode == retail_chex)	// [ML] Chex Quest mode - monsters don't drop items
    {
		return;
    }

	// Drop stuff.
	// This determines the kind of object spawned
	// during the death frame of a thing.
	mobjtype_t item = (mobjtype_t)0;

	//
	// sapientlion - if player killed themselves or were killed by the other
	// player(s), spawn a weapon (which they were helding moments before death) on
	// top of their remains.
	//
	// TODO add current ammo.
	//
	if (target->player)
	{
		if (sv_weapondrop)
		{
			switch (target->player->readyweapon)
			{
			case wp_pistol:
				item = MT_CLIP;
				break;

			case wp_shotgun:
				item = MT_SHOTGUN;
				break;

			case wp_chaingun:
				item = MT_CHAINGUN;
				break;

			case wp_missile:
				item = MT_MISC27; // Rocket launcher.
				break;

			case wp_plasma:
				item = MT_MISC28; // Plasma gun.
				break;

			case wp_bfg:
				item = MT_MISC25; // [RV] BFG.
				break;

			case wp_chainsaw:
				item = MT_MISC26; // Chainsaw.
				break;

			case wp_supershotgun:
				item = MT_SUPERSHOTGUN;
				break;

			default:
				return;
			}
		}
	}
	else
	{
		if (target->info->droppeditem != MT_NULL)
		{
			item = target->info->droppeditem;
		}
	}

	if (!item)
    {
		return; //Happens if bot or player killed when using fists.
    }

	// denis - dropped things will spawn serverside
	if (serverside)
	{
		mo = new AActor(target->x, target->y, ONFLOORZ, item);
		mo->flags |= MF_DROPPED;	// special versions of items
		SV_SpawnMobj(mo);
	}
}

static bool P_InfightingImmune(AActor* target, AActor* source)
{
	return // not default behaviour, and same group
		mobjinfo[target->type].infighting_group != IG_DEFAULT &&
		mobjinfo[target->type].infighting_group ==
		mobjinfo[source->type].infighting_group;
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//	creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//	creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
// [Toke] This is no longer needed client-side
void P_DamageMobj(AActor *target, AActor *inflictor, AActor *source, int damage, int mod, int flags)
{
	if (!serverside)
    {
		return;
    }

	player_t* player = target->player;
	player_t* splayer = NULL;
	if (source)
	{
		splayer = source->player;
	}

	if (!(target->flags & (MF_SHOOTABLE | MF_BOUNCES)))
    {
		return; // shouldn't happen...
    }

	// GhostlyDeath -- Spectators can't get hurt!
	if (player && player->spectator)
    {
		return;
    }

	if (target->health <= 0)
    {
		return;
    }

	// [AM] Target is invulnerable to infighting from any non-player source.
	if (source && source->player == NULL && target->oflags & MFO_INFIGHTINVUL)
	{
		return;
	}

	MeansOfDeath = mod;

	TeamInfo* teamInfo = NULL;
	bool targethasflag = false;
	team_t f = TEAM_NONE;

	if (player)
	{
		teamInfo = GetTeamInfo(player->userinfo.team);
		targethasflag = &idplayer(teamInfo->FlagData.flagger) == player;
		// Determine the team's flag the player has.
		if (targethasflag)
		{
			for (size_t i = 0; i < NUMTEAMS; i++)
			{
				if ((*player).flags[i])
				{
					f = (team_t)i;
				}
			}
		}
	}

	if (target->flags & MF_SKULLFLY)
	{
		target->momx = target->momy = target->momz = 0;
	}

	if (player)
    {
		damage = static_cast<int>(static_cast<float>(damage) * G_GetCurrentSkill().damage_factor);
    }

	// [AM] Weapon and monster damage scaling.
	if (source && source->player && target)
		damage *= sv_weapondamage;
	else if (source && target && player)
		damage *= sv_monsterdamage;

	// Some close combat weapons should not
	// inflict thrust and push the victim out of reach,
	// thus kick away unless using the chainsaw.

	if (inflictor && 
		!(target->flags & MF_NOCLIP) && 
	    (!source || !source->player || !(weaponinfo[source->player->readyweapon].flags & WPF_NOTHRUST)) &&
	    !(inflictor->flags2 & MF2_NODMGTHRUST))
	{

		unsigned int ang = P_PointToAngle(inflictor->x, inflictor->y, target->x, target->y);

		fixed_t thrust = damage * (FRACUNIT >> 3) * 100 / target->info->mass;

		// make fall forwards sometimes
		if (damage < 40
			&& damage > target->health
			&& target->z - inflictor->z > 64 * FRACUNIT
			&& (P_Random() & 1))
		{
			ang += ANG180;
			thrust *= 4;
		}

		ang >>= ANGLETOFINESHIFT;
		target->momx += FixedMul(thrust, finecosine[ang]);
		target->momy += FixedMul(thrust, finesine[ang]);
	}

	// player specific
	if (player)
	{
		short special = 11;
		if (map_format.getZDoom())
		{
			special = dDamage_End;
		}

		// end of game hell hack
		if (sv_gametype == GM_COOP || sv_allowexit)
		{
			if ((target->subsector->sector->special & 255) == special
				&& damage >= target->health)
			{
				damage = target->health - 1;
			}
		}

		// Below certain threshold,
		// ignore damage in GOD mode, or with INVUL power.
		if (damage < 1000 &&
		    ((player->cheats & CF_GODMODE) || player->powers[pw_invulnerability]))
		{
			return;
		}

		// [AM] No damage with sv_friendlyfire (was armor-only)
		if (!sv_friendlyfire && source && source->player && target != source &&
			mod != MOD_TELEFRAG)
		{
			if (G_IsCoopGame() || 
				(G_IsTeamGame() && player->userinfo.team == source->player->userinfo.team))
			{
				damage = 0;
			}
		}

		int armorDamage = 0;
		if (player->armortype && !(flags & DMG_NO_ARMOR))
		{
			if (player->armortype == deh.GreenAC)
				armorDamage = damage / 3;
			else
				armorDamage = damage / 2;

			if (player->armorpoints <= armorDamage)
			{
				// armor is used up
				armorDamage = player->armorpoints;
				player->armortype = 0;
			}
			player->armorpoints -= armorDamage;
			damage -= armorDamage;
		}

		// WDL damage events - they have to be up here to ensure we know how
		// much armor is subtracted.
		int low = std::max(target->health - damage, 0);
		int actualdamage = target->health - low;

		angle_t sangle = 0;

		if (splayer != NULL)
		{
			sangle = splayer->mo->angle / 4;
		}

		if (source == NULL && !targethasflag)
		{
			M_LogActorWDLEvent(WDL_EVENT_ENVIRODAMAGE, source, target, actualdamage,
			                   armorDamage, mod, 0);
		}
		else if (source == NULL && targethasflag)
		{
			M_LogActorWDLEvent(WDL_EVENT_ENVIROCARRIERDAMAGE, source, target,
			                   actualdamage, armorDamage, mod, f);
		}
		else if (source != NULL && targethasflag)
		{
			M_LogActorWDLEvent(WDL_EVENT_CARRIERDAMAGE, source, target, actualdamage,
			                   armorDamage, mod, f);
		}
		else
		{
			M_LogActorWDLEvent(WDL_EVENT_DAMAGE, source, target, actualdamage,
			                   armorDamage, mod, 0);
		}

		switch (mod)
		{
		case MOD_CHAINSAW:
		case MOD_FIST:
		case MOD_PISTOL:
		case MOD_CHAINGUN:
		case MOD_RAILGUN:
			M_LogWDLEvent(WDL_EVENT_SSACCURACY, splayer, player, sangle, mod, 1,
			              GetMaxShotsForMod(mod));
			break;
		case MOD_SHOTGUN:
		case MOD_SSHOTGUN:
			M_LogWDLEvent(WDL_EVENT_SPREADACCURACY, splayer, player, sangle, mod, 1,
			              GetMaxShotsForMod(mod));
			break;
		case MOD_ROCKET:
		case MOD_R_SPLASH:
		case MOD_BFG_BOOM:
		case MOD_PLASMARIFLE:
			M_LogWDLEvent(WDL_EVENT_PROJACCURACY, splayer, player, sangle, mod, 1,
			              GetMaxShotsForMod(mod));
			break;
		case MOD_BFG_SPLASH:
			M_LogWDLEvent(WDL_EVENT_TRACERACCURACY, splayer, player, sangle, mod, 1,
			              GetMaxShotsForMod(mod));
			break;
		}

		player->health -= damage; // mirror mobj health here for Dave
		target->health -= damage; // Do the same.

		if (player->health <= 0)
		{
			if (player->cheats & CF_BUDDHA && damage < 10000)
			{
				player->mo->health = player->health = target->health = 1;
			}
			else
			{
				player->health = 0;
			} 
		}

		player->attacker = source ? source->ptr() : AActor::AActorPtr();
		player->damagecount += damage; // add damage after armor / invuln

		if (player->damagecount > 100)
			player->damagecount = 100; // teleport stomp does 10k points...

		SV_SendDamagePlayer(player, inflictor, damage, armorDamage);
	}
	else // not player
	{
		// [RH] Only if not immune
		if (!(target->flags2 & (MF2_INVULNERABLE | MF2_DORMANT)))
		{
			// [AM] Armored monsters take less damage.
			if (target->oflags & MFO_ARMOR)
			{
				if (target->info->spawnhealth >= 1000)
				{
					// Big bodies get a green armor.
					damage = MAX((damage * 2) / 3, 1);
				}
				else
				{
					// Small bodies get a blue armor.
					damage = MAX(damage / 2, 1);
				}
			}

			// Calculate amount of HP to take away from the boss pool
			int low = std::max(target->health - damage, 0);
			int actualdamage = target->health - low;

			P_AddDamagePool(target, actualdamage);

			target->health -= damage; // do the damage to monsters.
			if (splayer)
			{
				if (target->health < 0)
				{
					if (P_GiveMonsterDamage(splayer, damage + target->health))
						PersistPlayerDamage(*splayer);
				}
				else
				{
					if (P_GiveMonsterDamage(splayer, damage))
						PersistPlayerDamage(*splayer);
				}
			}
		}
	}

	if (target->health <= 0)
	{
		// WDL damage events.
		// todo: handle voodoo dolls here
		if (source == NULL && targethasflag)
		{
			M_LogActorWDLEvent(WDL_EVENT_ENVIROCARRIERKILL, source, target, f, 0, mod, 0);
		}
		else if (source == NULL)
		{
			M_LogActorWDLEvent(WDL_EVENT_ENVIROKILL, source, target, 0, 0, mod, 0);
		}
		else if (targethasflag)
		{
			M_LogActorWDLEvent(WDL_EVENT_CARRIERKILL, source, target, f, 0, mod, 0);
		}
		else
		{
			M_LogActorWDLEvent(WDL_EVENT_KILL, source, target, 0, 0, mod, 0);
		}

		P_KillMobj(source, target, inflictor, false);

		return;
	}

    if (!(target->flags2 & MF2_DORMANT))
	{
		int pain = P_Random();

		if (target->oflags & MFO_UNFLINCHING)
		{
			// [AM] There is exactly one 255 in the random table.
			if (pain != 255 || target->info->painchance == 0)
			{
				// Force no flinch.
				pain = target->info->painchance;
			}
			else
			{
				// Force a flinch.
				pain = target->info->painchance - 1;
			}
		}

		if (!player)
		{
			SV_SendDamageMobj(target, pain);
		}
		if (pain < target->info->painchance &&
		    !(target->flags & MF_SKULLFLY) &&
		    !(player && !damage))
		{
			target->flags |= MF_JUSTHIT;	// fight back!
			P_SetMobjState(target, target->info->painstate);
		}

		target->reactiontime = 0;			// we're awake now...

		if (source &&
			source != target &&
		    !(source->flags3 & MF3_DMGIGNORED) &&
		    !(source->oflags & MFO_INFIGHTINVUL) &&
		    (!target->threshold || target->flags3 & MF3_NOTHRESHOLD) &&
		    !P_InfightingImmune(target, source))
		{
			// if not intent on another player, chase after this one
			// [AM] Infight invul monsters will never provoke attacks.

			// killough 2/15/98: remember last enemy, to prevent
			// sleeping early; 2/21/98: Place priority on players

			if (!target->lastenemy || !target->lastenemy->player ||
				target->lastenemy->health <= 0)
            {
				target->lastenemy = target->target; // remember last enemy - killough
            }

			target->target = source->ptr();
			target->threshold = BASETHRESHOLD;
			if (target->state == &states[target->info->spawnstate]
				&& target->info->seestate != S_NULL)
            {
				P_SetMobjState(target, target->info->seestate);
            }
            SV_ActorTarget(target);
		}
	}
}

//The player has left the game (in-game to spectator, or in-game disconnect)
void P_PlayerLeavesGame(player_s* player)
{
	if (level.behavior)
	{
		level.behavior->StartTypedScripts(SCRIPT_Disconnect, player->mo, player->GetPlayerNumber());
	}

	TeamInfo* teamInfo = NULL;
	bool targethasflag = false;
	team_t f = TEAM_NONE;
	team_t current = TEAM_NONE;
	if (player)
	{
		current = player->userinfo.team;
		teamInfo = GetTeamInfo(player->userinfo.team);
		targethasflag = &idplayer(teamInfo->FlagData.flagger) == player;
		// Determine the team's flag the player has.
		if (targethasflag)
		{
			for (size_t i = 0; i < NUMTEAMS; i++)
			{
				if ((*player).flags[i])
				{
					f = (team_t)i;
				}
			}
		}
	}

	if (targethasflag)
	{
		M_LogWDLEvent(WDL_EVENT_CARRIERKILL, player, player, f, 0, MOD_EXIT, 0);
	}
	else
	{
		M_LogWDLEvent(WDL_EVENT_KILL, player, player, 0, 0, MOD_EXIT, 0);
	}
	M_LogWDLEvent(WDL_EVENT_DISCONNECT, player, NULL, current, 0, 0, 0);

	// Playercount changes can cause end-of-game conditions.
	G_AssertValidPlayerCount();
}

void P_HealMobj(AActor* mo, int num)
{
	player_t* player = mo->player;

	if (mo->health <= 0 || (player && player->playerstate == PST_DEAD))
		return;

	if (player)
	{
		P_GiveBody(player, num);
		return;
	}
	else
	{
		int max = mobjinfo[mo->type].spawnhealth;

		mo->health += num;
		if (mo->health > max)
			mo->health = max;
	}
}

VERSION_CONTROL (p_interaction_cpp, "$Id$")
