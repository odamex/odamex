// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//   Server message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <bitset>

#include "svc_message.h"

#include "common.pb.h"
#include "d_main.h"
#include "g_gametype.h"
#include "i_system.h"
#include "m_strindex.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_unlag.h"

/**
 * @brief Pack an array of booleans into a bitfield.
 */
static uint32_t PackBoolArray(const bool* bools, size_t count)
{
	uint32_t out = 0;
	for (size_t i = 0; i < count; i++)
	{
		if (bools[i])
		{
			out |= (1 << i);
		}
	}
	return out;
}

odaproto::svc::Disconnect SVC_Disconnect(const char* message)
{
	odaproto::svc::Disconnect msg;

	if (message != NULL)
	{
		msg.set_message(message);
	}

	return msg;
}

/**
 * @brief Send information about a player.
 */
odaproto::svc::PlayerInfo SVC_PlayerInfo(player_t& player)
{
	odaproto::svc::PlayerInfo msg;

	uint32_t packedweapons = PackBoolArray(player.weaponowned, NUMWEAPONS);
	msg.mutable_player()->set_weaponowned(packedweapons);

	uint32_t packedcards = PackBoolArray(player.cards, NUMCARDS);
	msg.mutable_player()->set_cards(packedcards);

	msg.mutable_player()->set_backpack(player.backpack);

	for (int i = 0; i < NUMAMMO; i++)
	{
		msg.mutable_player()->add_ammo(player.ammo[i]);
		msg.mutable_player()->add_maxammo(player.maxammo[i]);
	}

	msg.mutable_player()->set_health(player.health);
	msg.mutable_player()->set_armorpoints(player.armorpoints);
	msg.mutable_player()->set_armortype(player.armortype);
	msg.mutable_player()->set_lives(player.lives);
	msg.mutable_player()->set_readyweapon(player.readyweapon);
	msg.mutable_player()->set_pendingweapon(player.pendingweapon);

	for (int i = 0; i < NUMPOWERS; i++)
	{
		msg.mutable_player()->add_powers(player.powers[i]);
	}

	if (!player.spectator)
		msg.mutable_player()->set_cheats(player.cheats);

	return msg;
}

/**
 * @brief Change the location of a player.
 */
odaproto::svc::MovePlayer SVC_MovePlayer(player_t& player, const int tic)
{
	odaproto::svc::MovePlayer msg;

	odaproto::Actor* act = msg.mutable_actor();
	odaproto::Player* pl = msg.mutable_player();

	pl->set_playerid(player.id); // player number

	// [SL] 2011-09-14 - the most recently processed ticcmd from the
	// client we're sending this message to.
	msg.set_tic(tic);

	odaproto::Vec3* pos = act->mutable_pos();
	pos->set_x(player.mo->x);
	pos->set_y(player.mo->y);
	pos->set_z(player.mo->z);

	act->set_angle(player.mo->angle);
	act->set_pitch(player.mo->pitch);

	if (player.mo->frame == 32773)
	{
		msg.set_frame(PLAYER_FULLBRIGHTFRAME);
	}
	else
	{
		msg.set_frame(player.mo->frame);
	}

	// write velocity
	odaproto::Vec3* mom = act->mutable_mom();
	mom->set_x(player.mo->momx);
	mom->set_y(player.mo->momy);
	mom->set_z(player.mo->momz);

	// [Russell] - hack, tell the client about the partial
	// invisibility power of another player.. (cheaters can disable
	// this but its all we have for now)
	pl->mutable_powers()->Resize(pw_invisibility + 1, 0);
	pl->set_powers(pw_invisibility, player.powers[pw_invisibility]);

	return msg;
}

/**
 * @brief Send the local player position for a client.
 */
odaproto::svc::UpdateLocalPlayer SVC_UpdateLocalPlayer(AActor& mo, const int tic)
{
	odaproto::svc::UpdateLocalPlayer msg;

	// client player will update his position if packets were missed
	odaproto::Actor* act = msg.mutable_actor();

	// client-tic of the most recently processed ticcmd for this client
	msg.set_tic(tic);

	odaproto::Vec3* pos = act->mutable_pos();
	pos->set_x(mo.x);
	pos->set_y(mo.y);
	pos->set_z(mo.z);

	odaproto::Vec3* mom = act->mutable_mom();
	mom->set_x(mo.momx);
	mom->set_y(mo.momy);
	mom->set_z(mo.momz);

	act->set_waterlevel(mo.waterlevel);

	return msg;
}

/**
 * @brief Persist level locals to the client.
 *
 * @param b Buffer to write to.
 * @param locals Level locals struct to send.
 * @param flags SVC_LL_* bit flags to designate what gets sent.
 */
odaproto::svc::LevelLocals SVC_LevelLocals(const level_locals_t& locals, uint32_t flags)
{
	odaproto::svc::LevelLocals msg;

	msg.set_flags(flags);

	if (flags & SVC_LL_TIME)
	{
		msg.set_time(locals.time);
	}

	if (flags & SVC_LL_TOTALS)
	{
		msg.set_total_secrets(locals.total_secrets);
		msg.set_total_items(locals.total_items);
		msg.set_total_monsters(locals.total_monsters);
	}

	if (flags & SVC_LL_SECRETS)
	{
		msg.set_found_secrets(locals.found_secrets);
	}

	if (flags & SVC_LL_ITEMS)
	{
		msg.set_found_items(locals.found_items);
	}

	if (flags & SVC_LL_MONSTERS)
	{
		msg.set_killed_monsters(locals.killed_monsters);
	}

	if (flags & SVC_LL_MONSTER_RESPAWNS)
	{
		msg.set_respawned_monsters(locals.respawned_monsters);
	}

	return msg;
}

/**
 * @brief Send the server's current time in MS to the client.
 */
odaproto::svc::PingRequest SVC_PingRequest()
{
	odaproto::svc::PingRequest msg;

	msg.set_ms_time(I_MSTime());

	return msg;
}

odaproto::svc::UpdatePing SVC_UpdatePing(player_t& player)
{
	odaproto::svc::UpdatePing msg;

	msg.set_pid(player.id);
	msg.set_ping(player.ping);

	return msg;
}

odaproto::svc::SpawnMobj SVC_SpawnMobj(AActor* mo)
{
	odaproto::svc::SpawnMobj msg;

	odaproto::Actor* base = msg.mutable_baseline();
	odaproto::Vec3* bpos = base->mutable_pos();
	odaproto::Vec3* bmom = base->mutable_mom();

	odaproto::Actor* cur = msg.mutable_current();
	odaproto::Vec3* apos = cur->mutable_pos();
	odaproto::Vec3* amom = cur->mutable_mom();

	uint32_t spawnFlags = 0;

	// Contents of the baseline

	bpos->set_x(mo->baseline.pos.x);
	bpos->set_y(mo->baseline.pos.y);
	bpos->set_z(mo->baseline.pos.z);
	bmom->set_x(mo->baseline.mom.x);
	bmom->set_y(mo->baseline.mom.y);
	bmom->set_z(mo->baseline.mom.z);
	base->set_angle(mo->baseline.angle);
	base->set_targetid(mo->baseline.targetid);
	base->set_tracerid(mo->baseline.tracerid);
	base->set_movecount(mo->baseline.movecount);
	base->set_movedir(mo->baseline.movedir);
	base->set_rndindex(mo->baseline.rndindex);

	// Current location could be different than the baseline

	uint32_t bflags = P_GetMobjBaselineFlags(*mo);
	msg.set_baseline_flags(bflags);

	if (bflags & baseline_t::POSX)
	{
		apos->set_x(mo->x);
	}
	if (bflags & baseline_t::POSY)
	{
		apos->set_y(mo->y);
	}
	if (bflags & baseline_t::POSZ)
	{
		apos->set_z(mo->z);
	}
	if (bflags & baseline_t::ANGLE)
	{
		cur->set_angle(mo->angle);
	}
	if (bflags & baseline_t::MOVEDIR)
	{
		cur->set_movedir(mo->movedir);
	}
	if (bflags & baseline_t::MOVECOUNT)
	{
		cur->set_movecount(mo->movecount);
	}
	if (bflags & baseline_t::RNDINDEX)
	{
		cur->set_rndindex(mo->rndindex);
	}
	if (bflags & baseline_t::TARGET)
	{
		cur->set_targetid(mo->target ? mo->target->netid : 0);
	}
	if (bflags & baseline_t::TRACER)
	{
		cur->set_tracerid(mo->tracer ? mo->tracer->netid : 0);
	}
	if (bflags & baseline_t::MOMX)
	{
		amom->set_x(mo->momx);
	}
	if (bflags & baseline_t::MOMY)
	{
		amom->set_y(mo->momy);
	}
	if (bflags & baseline_t::MOMZ)
	{
		amom->set_z(mo->momz);
	}

	cur->set_type(mo->type);
	cur->set_netid(mo->netid);

	// denis - sending state fixes monster ghosts appearing under doors
	cur->set_statenum(mo->state - states);

	if (mo->type == MT_FOUNTAIN)
	{
		msg.add_args(mo->args[0]);
	}

	if (mo->type == MT_ZDOOMBRIDGE)
	{
		msg.add_args(mo->args[0]);
		msg.add_args(mo->args[1]);
	}

	// denis - check type as that is what the client will be spawning
	if (mo->flags & MF_MISSILE || mobjinfo[mo->type].flags & MF_MISSILE)
	{
		msg.set_target_netid(mo->target ? mo->target->netid : 0);
	}
	else if (mo->flags & MF_AMBUSH || mo->flags & MF_DROPPED)
	{
		spawnFlags |= SVC_SM_FLAGS;
		cur->set_flags(mo->flags);
	}

	// odamex flags - only monster flags for now
	if (mo->oflags & hordeBossModMask)
	{
		spawnFlags |= SVC_SM_OFLAGS;
		cur->set_oflags(mo->oflags & hordeBossModMask);
	}

	// animating corpses
	if ((mo->flags & MF_CORPSE) && mo->state - states != S_GIBS)
	{
		// This sets off some additional logic on the client.
		spawnFlags |= SVC_SM_CORPSE;
		cur->set_frame(mo->frame);
		cur->set_tics(mo->tics);
	}

	msg.set_spawn_flags(spawnFlags);

	return msg;
}

odaproto::svc::DisconnectClient SVC_DisconnectClient(player_t& player)
{
	odaproto::svc::DisconnectClient msg;

	msg.set_pid(player.id);

	return msg;
}

/**
 * @brief Sends a message to a player telling them to change to the specified
 *        WAD and DEH patch files and load a map.
 */
odaproto::svc::LoadMap SVC_LoadMap(const OResFiles& wadnames, const OResFiles& patchnames,
                                   const std::string& mapname, int time)
{
	odaproto::svc::LoadMap msg;

	// send list of wads (skip over wadnames[0] == odamex.wad)
	size_t wadcount = wadnames.size() - 1;
	for (size_t i = 1; i < wadcount + 1; i++)
	{
		odaproto::svc::LoadMap_Resource* wad = msg.add_wadnames();
		wad->set_name(wadnames[i].getBasename());
		wad->set_hash(wadnames[i].getMD5().getHexStr());
	}

	// send list of DEH/BEX patches
	size_t patchcount = patchnames.size();
	for (size_t i = 0; i < patchcount; i++)
	{
		odaproto::svc::LoadMap_Resource* patch = msg.add_patchnames();
		patch->set_name(patchnames[i].getBasename());
		patch->set_hash(patchnames[i].getMD5().getHexStr());
	}

	msg.set_mapname(mapname);
	msg.set_time(time);

	return msg;
}

odaproto::svc::ConsolePlayer SVC_ConsolePlayer(player_t& player,
                                               const std::string& digest)
{
	odaproto::svc::ConsolePlayer msg;

	msg.set_pid(player.id);
	msg.set_digest(digest);

	return msg;
}

odaproto::svc::ExplodeMissile SVC_ExplodeMissile(AActor& mobj)
{
	odaproto::svc::ExplodeMissile msg;

	msg.set_netid(mobj.netid);

	return msg;
}

odaproto::svc::RemoveMobj SVC_RemoveMobj(AActor& mobj)
{
	odaproto::svc::RemoveMobj msg;

	msg.set_netid(mobj.netid);

	return msg;
}

odaproto::svc::UserInfo SVC_UserInfo(player_t& player, int64_t time)
{
	odaproto::svc::UserInfo msg;

	msg.set_pid(player.id);
	msg.set_netname(player.userinfo.netname);
	msg.set_team(player.userinfo.team);
	msg.set_gender(player.userinfo.gender);
	msg.set_colorpreset(player.userinfo.colorpreset);

	// [AM] Alpha is always 255.
	odaproto::Color* color = msg.mutable_color();
	color->set_r(player.userinfo.color[1]);
	color->set_g(player.userinfo.color[2]);
	color->set_b(player.userinfo.color[3]);

	msg.set_join_time(time);

	return msg;
}

/**
 * @brief Update mobj data on the client compared to the baseline.
 */
odaproto::svc::UpdateMobj SVC_UpdateMobj(AActor& mobj)
{
	odaproto::svc::UpdateMobj msg;

	uint32_t flags = P_GetMobjBaselineFlags(mobj);
	msg.set_flags(flags);

	odaproto::Actor* act = msg.mutable_actor();
	odaproto::Vec3* pos = act->mutable_pos();
	odaproto::Vec3* mom = act->mutable_mom();

	act->set_netid(mobj.netid);

	if (flags & baseline_t::POSX)
	{
		pos->set_x(mobj.x);
	}
	if (flags & baseline_t::POSY)
	{
		pos->set_y(mobj.y);
	}
	if (flags & baseline_t::POSZ)
	{
		pos->set_z(mobj.z);
	}
	if (flags & baseline_t::ANGLE)
	{
		act->set_angle(mobj.angle);
	}
	if (flags & baseline_t::MOVEDIR)
	{
		act->set_movedir(mobj.movedir);
	}
	if (flags & baseline_t::MOVECOUNT)
	{
		act->set_movecount(mobj.movecount);
	}
	if (flags & baseline_t::RNDINDEX)
	{
		act->set_rndindex(mobj.rndindex);
	}
	if (flags & baseline_t::TARGET)
	{
		act->set_targetid(mobj.target ? mobj.target->netid : 0);
	}
	if (flags & baseline_t::TRACER)
	{
		act->set_tracerid(mobj.tracer ? mobj.tracer->netid : 0);
	}
	if (flags & baseline_t::MOMX)
	{
		mom->set_x(mobj.momx);
	}
	if (flags & baseline_t::MOMY)
	{
		mom->set_y(mobj.momy);
	}
	if (flags & baseline_t::MOMZ)
	{
		mom->set_z(mobj.momz);
	}

	return msg;
}

odaproto::svc::SpawnPlayer SVC_SpawnPlayer(player_t& player)
{
	odaproto::svc::SpawnPlayer msg;

	msg.set_pid(player.id);

	odaproto::Actor* act = msg.mutable_actor();
	if (player.mo)
	{
		act->set_netid(player.mo->netid);
		act->set_angle(player.mo->angle);
		act->mutable_pos()->set_x(player.mo->x);
		act->mutable_pos()->set_y(player.mo->y);
		act->mutable_pos()->set_z(player.mo->z);
	}
	else
	{
		// The client hasn't yet received his own position from the server
		// This happens with cl_autorecord
		// Just fake a position for now
		act->set_netid(MAXSHORT);
	}

	return msg;
}

odaproto::svc::DamagePlayer SVC_DamagePlayer(player_t& player, AActor* inflictor, int health, int armor)
{
	odaproto::svc::DamagePlayer msg;

	msg.set_netid(player.mo->netid);
	msg.set_inflictorid(inflictor ? inflictor->netid : 0);
	msg.set_health_damage(health);
	msg.set_armor_damage(armor);
	msg.mutable_player()->set_health(player.health);
	msg.mutable_player()->set_armorpoints(player.armorpoints);

	return msg;
}

/**
 * @brief Kill a mobj.
 */
odaproto::svc::KillMobj SVC_KillMobj(AActor* source, AActor* target, AActor* inflictor,
                                     int mod, bool joinkill)
{
	odaproto::svc::KillMobj msg;

	odaproto::Actor* tgt = msg.mutable_target();

	msg.set_source_netid(source ? source->netid : 0);
	msg.set_inflictor_netid(inflictor ? inflictor->netid : 0);
	msg.set_health(target->health);
	msg.set_mod(mod);
	msg.set_joinkill(joinkill);

	// [AM] Confusingly, we send the lives _before_ we take it away, so
	//      the lives logic can live in the kill function.
	msg.set_lives(target->player ? target->player->lives : -1);

	// send death location first
	tgt->set_netid(target->netid);
	tgt->set_rndindex(target->rndindex);

	// [SL] 2012-12-26 - Get real position since this actor is at
	// a reconciled position due to unlag
	fixed_t xoffs = 0, yoffs = 0, zoffs = 0;
	if (target->player)
	{
		Unlag::getInstance().getReconciliationOffset(target->player->id, xoffs, yoffs,
		                                             zoffs);
	}

	odaproto::Vec3* tgtpos = tgt->mutable_pos();
	tgtpos->set_x(target->x + xoffs);
	tgtpos->set_y(target->y + yoffs);
	tgtpos->set_z(target->z + zoffs);

	tgt->set_angle(target->angle);

	odaproto::Vec3* tgtmom = tgt->mutable_mom();
	tgtmom->set_x(target->momx);
	tgtmom->set_y(target->momy);
	tgtmom->set_z(target->momz);

	return msg;
}

odaproto::svc::FireWeapon SVC_FireWeapon(player_t& player)
{
	odaproto::svc::FireWeapon msg;

	msg.set_readyweapon(player.readyweapon);
	msg.set_servertic(player.tic);

	return msg;
}

odaproto::svc::UpdateSector SVC_UpdateSector(sector_t& sector)
{
	odaproto::svc::UpdateSector msg;

	msg.set_sectornum(&sector - ::sectors);
	odaproto::Sector* secmsg = msg.mutable_sector();

	secmsg->set_floor_height(P_FloorHeight(&sector));
	secmsg->set_ceiling_height(P_CeilingHeight(&sector));
	secmsg->set_floorpic(sector.floorpic);
	secmsg->set_ceilingpic(sector.ceilingpic);
	secmsg->set_special(sector.special);

	return msg;
}

odaproto::svc::Print SVC_Print(printlevel_t level, const std::string& str)
{
	odaproto::svc::Print msg;

	msg.set_level(level);
	msg.set_message(str);

	return msg;
}

/**
 * @brief Persist player information to the client that is less vital.
 *
 * @param b Buffer to write to.
 * @param player Player to send information about.
 * @param flags SVC_PM_* flags to designate what gets sent.
 */
odaproto::svc::PlayerMembers SVC_PlayerMembers(player_t& player, byte flags)
{
	odaproto::svc::PlayerMembers msg;

	msg.set_pid(player.id);
	msg.set_flags(flags);

	if (flags & SVC_PM_SPECTATOR)
	{
		msg.set_spectator(player.spectator);
	}

	if (flags & SVC_PM_READY)
	{
		msg.set_ready(player.ready);
	}

	if (flags & SVC_PM_LIVES)
	{
		msg.set_lives(player.lives);
	}

	if (flags & SVC_PM_DAMAGE)
	{
		msg.set_monsterdmgcount(player.monsterdmgcount);
	}

	if (flags & SVC_PM_SCORE)
	{
		// [AM] Just send everything instead of trying to be clever about
		//      what we send based on game modes.  There's less room for
		//      breakage when adding game modes, and we have varints now,
		//      which means we're probably saving bandwidth anyway.
		msg.set_roundwins(player.roundwins);
		msg.set_points(player.points);
		msg.set_fragcount(player.fragcount);
		msg.set_deathcount(player.deathcount);
		msg.set_killcount(player.killcount);
		// [AM] Is there any reason we would ever care about itemcount?
		msg.set_secretcount(player.secretcount);
		msg.set_totalpoints(player.totalpoints);
		msg.set_totaldeaths(player.totaldeaths);
	}

	if (flags & SVC_PM_CHEATS)
	{
		if (!player.spectator)
			msg.set_cheats(player.cheats);
	}

	return msg;
}

/**
 * Persist mutable team information to the client.
 */
odaproto::svc::TeamMembers SVC_TeamMembers(team_t team)
{
	odaproto::svc::TeamMembers msg;

	TeamInfo* info = GetTeamInfo(team);

	msg.set_team(team);
	msg.set_points(info->Points);
	msg.set_roundwins(info->RoundWins);

	return msg;
}

odaproto::svc::ActivateLine SVC_ActivateLine(line_t* line, AActor* mo, int side,
                                             LineActivationType type)
{
	odaproto::svc::ActivateLine msg;

	msg.set_linenum(line ? line - ::lines : -1);
	msg.set_activator_netid(mo ? mo->netid : 0);
	msg.set_side(side);
	msg.set_activation_type(type);

	return msg;
}

odaproto::svc::MovingSector SVC_MovingSector(const sector_t& sector)
{
	odaproto::svc::MovingSector msg;

	ptrdiff_t sectornum = &sector - ::sectors;

	// Determine which moving planes are in this sector - governs which
	// fields to send to the other side.
	movertype_t floor_mover = SEC_INVALID, ceiling_mover = SEC_INVALID;

	if (sector.ceilingdata && sector.ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
	{
		ceiling_mover = SEC_CEILING;
	}
	if (sector.ceilingdata && sector.ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
	{
		ceiling_mover = SEC_DOOR;
	}
	if (sector.floordata && sector.floordata->IsA(RUNTIME_CLASS(DFloor)))
	{
		floor_mover = SEC_FLOOR;
	}
	if (sector.floordata && sector.floordata->IsA(RUNTIME_CLASS(DPlat)))
	{
		floor_mover = SEC_PLAT;
	}
	if (sector.ceilingdata && sector.ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
	{
		ceiling_mover = SEC_ELEVATOR;
		floor_mover = SEC_INVALID;
	}
	if (sector.ceilingdata && sector.ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
	{
		ceiling_mover = SEC_PILLAR;
		floor_mover = SEC_INVALID;
	}

	// no moving planes?  skip it.
	if (ceiling_mover == SEC_INVALID && floor_mover == SEC_INVALID)
	{
		return msg;
	}

	// Create bitfield to denote moving planes in this sector
	byte movers = byte(ceiling_mover) | (byte(floor_mover) << 4);

	msg.set_sector(sectornum);
	msg.set_ceiling_height(P_CeilingHeight(&sector));
	msg.set_floor_height(P_FloorHeight(&sector));
	msg.set_movers(movers);

	if (ceiling_mover == SEC_ELEVATOR)
	{
		DElevator* Elevator = static_cast<DElevator*>(sector.ceilingdata);

		odaproto::svc::MovingSector_Snapshot* ceil = msg.mutable_ceiling_mover();
		ceil->set_ceil_type(Elevator->m_Type);
		ceil->set_ceil_status(Elevator->m_Status);
		ceil->set_ceil_dir(Elevator->m_Direction);
		ceil->set_floor_dest(Elevator->m_FloorDestHeight);
		ceil->set_ceil_dest(Elevator->m_CeilingDestHeight);
		ceil->set_ceil_speed(Elevator->m_Speed);
	}

	if (ceiling_mover == SEC_PILLAR)
	{
		DPillar* Pillar = static_cast<DPillar*>(sector.ceilingdata);

		odaproto::svc::MovingSector_Snapshot* ceil = msg.mutable_ceiling_mover();
		ceil->set_ceil_type(Pillar->m_Type);
		ceil->set_ceil_status(Pillar->m_Status);
		ceil->set_floor_speed(Pillar->m_FloorSpeed);
		ceil->set_ceil_speed(Pillar->m_CeilingSpeed);
		ceil->set_floor_dest(Pillar->m_FloorTarget);
		ceil->set_ceil_dest(Pillar->m_CeilingTarget);
		ceil->set_ceil_crush(Pillar->m_Crush);
	}

	if (ceiling_mover == SEC_CEILING)
	{
		DCeiling* Ceiling = static_cast<DCeiling*>(sector.ceilingdata);

		odaproto::svc::MovingSector_Snapshot* ceil = msg.mutable_ceiling_mover();
		ceil->set_ceil_type(Ceiling->m_Type);
		ceil->set_ceil_low(Ceiling->m_BottomHeight);
		ceil->set_ceil_high(Ceiling->m_TopHeight);
		ceil->set_ceil_speed(Ceiling->m_Speed);
		ceil->set_crusher_speed_1(Ceiling->m_Speed1);
		ceil->set_crusher_speed_2(Ceiling->m_Speed2);
		ceil->set_ceil_crush(Ceiling->m_Crush);
		ceil->set_silent(Ceiling->m_Silent);
		ceil->set_ceil_dir(Ceiling->m_Direction);
		ceil->set_ceil_tex(Ceiling->m_Texture);
		ceil->set_ceil_new_special(Ceiling->m_NewSpecial);
		ceil->set_ceil_tag(Ceiling->m_Tag);
		ceil->set_ceil_old_dir(Ceiling->m_OldDirection);
	}

	if (ceiling_mover == SEC_DOOR)
	{
		DDoor* Door = static_cast<DDoor*>(sector.ceilingdata);

		odaproto::svc::MovingSector_Snapshot* ceil = msg.mutable_ceiling_mover();
		ceil->set_ceil_type(Door->m_Type);
		ceil->set_ceil_height(Door->m_TopHeight);
		ceil->set_ceil_speed(Door->m_Speed);
		ceil->set_ceil_wait(Door->m_TopWait);
		ceil->set_ceil_counter(Door->m_TopCountdown);
		ceil->set_ceil_status(Door->m_Status);
		// Check for an invalid m_Line (doors triggered by tag 666)
		ceil->set_ceil_line(Door->m_Line ? (Door->m_Line - lines) : -1);
	}

	if (floor_mover == SEC_FLOOR)
	{
		DFloor* Floor = static_cast<DFloor*>(sector.floordata);

		odaproto::svc::MovingSector_Snapshot* floor = msg.mutable_floor_mover();
		floor->set_floor_type(Floor->m_Type);
		floor->set_floor_status(Floor->m_Status);
		floor->set_floor_crush(Floor->m_Crush);
		floor->set_floor_dir(Floor->m_Direction);
		floor->set_floor_speed(Floor->m_NewSpecial);
		floor->set_floor_tex(Floor->m_Texture);
		floor->set_floor_dest(Floor->m_FloorDestHeight);
		floor->set_floor_speed(Floor->m_Speed);
		floor->set_reset_counter(Floor->m_ResetCount);
		floor->set_orig_height(Floor->m_OrgHeight);
		floor->set_delay(Floor->m_Delay);
		floor->set_pause_time(Floor->m_PauseTime);
		floor->set_step_time(Floor->m_StepTime);
		floor->set_per_step_time(Floor->m_PerStepTime);
		floor->set_floor_offset(Floor->m_Height);
		floor->set_floor_change(Floor->m_Change);
		floor->set_floor_line(Floor->m_Line ? (Floor->m_Line - lines) : -1);
	}

	if (floor_mover == SEC_PLAT)
	{
		DPlat* Plat = static_cast<DPlat*>(sector.floordata);

		odaproto::svc::MovingSector_Snapshot* floor = msg.mutable_floor_mover();
		floor->set_floor_speed(Plat->m_Speed);
		floor->set_floor_low(Plat->m_Low);
		floor->set_floor_high(Plat->m_High);
		floor->set_floor_wait(Plat->m_Wait);
		floor->set_floor_counter(Plat->m_Count);
		floor->set_floor_status(Plat->m_Status);
		floor->set_floor_old_status(Plat->m_OldStatus);
		floor->set_floor_crush(Plat->m_Crush);
		floor->set_floor_tag(Plat->m_Tag);
		floor->set_floor_type(Plat->m_Type);
		floor->set_floor_offset(Plat->m_Height);
		floor->set_floor_lip(Plat->m_Lip);
	}

	return msg;
}

odaproto::svc::PlaySound SVC_PlaySound(const PlaySoundType& type, int channel, int sfx_id,
                                       float volume, int attenuation)
{
	odaproto::svc::PlaySound msg;

	msg.set_channel(channel);
	msg.set_sfxid(sfx_id);
	msg.set_volume(volume);
	msg.set_attenuation(attenuation);

	switch (type.tag)
	{
	case PlaySoundType::PS_NONE:
		break;
	case PlaySoundType::PS_MOBJ:
		msg.set_netid(type.data.mo->netid);
		break;
	case PlaySoundType::PS_POS:
		msg.mutable_pos()->set_x(type.data.pos.x);
		msg.mutable_pos()->set_y(type.data.pos.y);
		break;
	}

	return msg;
}

odaproto::svc::TouchSpecial SVC_TouchSpecial(AActor* mo)
{
	odaproto::svc::TouchSpecial msg;

	msg.set_netid(mo->netid);

	return msg;
}

/**
 * @brief Send information about a player
 */
odaproto::svc::PlayerState SVC_PlayerState(player_t& player)
{
	odaproto::svc::PlayerState msg;

	odaproto::Player* pl = msg.mutable_player();

	pl->set_playerid(player.id);
	pl->set_health(player.health);
	pl->set_armortype(player.armortype);
	pl->set_armorpoints(player.armorpoints);
	pl->set_lives(player.lives);
	pl->set_readyweapon(player.readyweapon);

	std::bitset<6> cardBits;
	for (int i = 0; i < NUMCARDS; i++)
	{
		cardBits.set(i, player.cards[i]);
	}
	pl->set_cards(cardBits.to_ulong());

	for (int i = 0; i < NUMAMMO; i++)
	{
		pl->add_ammo(player.ammo[i]);
	}

	for (int i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t* psp = &player.psprites[i];
		unsigned int state = psp->state - states;
		odaproto::Player_Psp* plpsp = pl->add_psprites();
		plpsp->set_statenum(state);
	}

	for (int i = 0; i < NUMPOWERS; i++)
	{
		pl->add_powers(player.powers[i]);
	}

	if (!player.spectator)
		pl->set_cheats(player.cheats);

	return msg;
}

/**
 * @brief Send information about the server's LevelState to the client.
 */
odaproto::svc::LevelState SVC_LevelState(const SerializedLevelState& sls)
{
	odaproto::svc::LevelState msg;

	msg.set_state(sls.state);
	msg.set_countdown_done_time(sls.countdown_done_time);
	msg.set_ingame_start_time(sls.ingame_start_time);
	msg.set_round_number(sls.round_number);
	msg.set_last_wininfo_type(sls.last_wininfo_type);
	msg.set_last_wininfo_id(sls.last_wininfo_id);

	return msg;
}

odaproto::svc::PlayerQueuePos SVC_PlayerQueuePos(const player_t& source)
{
	odaproto::svc::PlayerQueuePos msg;

	msg.set_pid(source.id);
	msg.set_queuepos(source.QueuePosition);

	return msg;
}

odaproto::svc::ForceTeam SVC_ForceTeam(team_t team)
{
	odaproto::svc::ForceTeam msg;

	msg.set_team(team);

	return msg;
}

odaproto::svc::Switch SVC_Switch(line_t& line, uint32_t state, uint32_t timer)
{
	odaproto::svc::Switch msg;

	msg.set_linenum(&line - ::lines);
	msg.set_switch_active(line.switchactive);
	msg.set_special(line.special);
	msg.set_state(state);
	msg.set_button_texture(P_GetButtonTexture(&line));
	msg.set_timer(timer);

	return msg;
}

odaproto::svc::Say SVC_Say(const bool visibility, const byte pid,
                           const std::string& message)
{
	odaproto::svc::Say msg;

	msg.set_visibility(visibility);
	msg.set_pid(pid);
	msg.set_message(message);

	return msg;
}

odaproto::svc::CTFRefresh SVC_CTFRefresh(const TeamsView& teams, const bool full)
{
	odaproto::svc::CTFRefresh msg;

	msg.set_full(full);

	for (TeamsView::const_iterator it = teams.begin(); it != teams.end(); ++it)
	{
		odaproto::svc::CTFRefresh_TeamInfo* info = msg.add_team_info();

		info->set_points((*it)->Points);

		if (full)
		{
			info->set_flag_state((*it)->FlagData.state);
			info->set_flag_flagger((*it)->FlagData.flagger);
		}
	}

	return msg;
}

odaproto::svc::CTFEvent SVC_CTFEvent(const flag_score_t event, const team_t target,
                                     const player_t& player)
{
	odaproto::svc::CTFEvent msg;

	msg.set_event(event);
	msg.set_target_team(target);

	// [AM] FIXME: validplayer shouldn't need a const I don't think...
	if (validplayer(const_cast<player_t&>(player)))
	{
		msg.set_player_team(player.userinfo.team);
		msg.set_player_id(player.id);
		if (G_IsRoundsGame())
		{
			msg.set_player_totalpoints(player.totalpoints);
		}
		else
		{
			msg.set_player_points(player.points);
		}
	}

	return msg;
}

/**
 * @brief Send information about a player who discovered a secret.
 */
odaproto::svc::SecretEvent SVC_SecretEvent(player_t& player, sector_t& sector)
{
	odaproto::svc::SecretEvent msg;

	msg.set_pid(player.id);
	msg.set_sectornum(&sector - ::sectors);
	msg.mutable_sector()->set_special(sector.special);

	return msg;
}

odaproto::svc::ServerSettings SVC_ServerSettings(const cvar_t& var)
{
	odaproto::svc::ServerSettings msg;

	msg.set_key(var.name());
	msg.set_value(var.str());

	return msg;
}

odaproto::svc::ConnectClient SVC_ConnectClient(const player_t& player)
{
	odaproto::svc::ConnectClient msg;

	msg.set_pid(player.id);

	return msg;
}

odaproto::svc::MidPrint SVC_MidPrint(const std::string& message, const int time)
{
	odaproto::svc::MidPrint msg;

	msg.set_message(message);
	msg.set_time(time);

	return msg;
}

odaproto::svc::ServerGametic SVC_ServerGametic(const byte tic)
{
	odaproto::svc::ServerGametic msg;

	msg.set_tic(tic);

	return msg;
}

odaproto::svc::IntTimeLeft SVC_IntTimeLeft(const unsigned int timeleft)
{
	odaproto::svc::IntTimeLeft msg;

	msg.set_timeleft(timeleft);

	return msg;
}

odaproto::svc::RailTrail SVC_RailTrail(const v3double_t& start, const v3double_t& end)
{
	odaproto::svc::RailTrail msg;

	odaproto::Vec3* stmsg = msg.mutable_start();
	stmsg->set_x(DOUBLE2FIXED(start.x));
	stmsg->set_y(DOUBLE2FIXED(start.y));
	stmsg->set_z(DOUBLE2FIXED(start.z));

	odaproto::Vec3* edmsg = msg.mutable_end();
	edmsg->set_x(DOUBLE2FIXED(end.x));
	edmsg->set_y(DOUBLE2FIXED(end.y));
	edmsg->set_z(DOUBLE2FIXED(end.z));

	return msg;
}

odaproto::svc::LineUpdate SVC_LineUpdate(const line_t& line)
{
	odaproto::svc::LineUpdate msg;

	msg.set_linenum(&line - ::lines);
	msg.set_flags(line.flags);
	msg.set_lucency(line.lucency);

	return msg;
}

odaproto::svc::SectorProperties SVC_SectorProperties(sector_t& sector)
{
	odaproto::svc::SectorProperties msg;

	msg.set_sectornum(&sector - ::sectors);
	msg.set_changes(sector.SectorChanges);
	odaproto::Sector* secmsg = msg.mutable_sector();

	for (int i = 0, prop = 1; prop < SPC_Max; i++)
	{
		prop = 1 << i;
		if ((prop & sector.SectorChanges) == 0)
			continue;

		switch (prop)
		{
		case SPC_FlatPic:
			secmsg->set_floorpic(sector.floorpic);
			secmsg->set_ceilingpic(sector.ceilingpic);
			break;
		case SPC_LightLevel:
			secmsg->set_lightlevel(sector.lightlevel);
			break;
		case SPC_Color: {
			odaproto::Color* color = secmsg->mutable_colormap()->mutable_color();
			color->set_r(sector.colormap->color.getr());
			color->set_g(sector.colormap->color.getg());
			color->set_b(sector.colormap->color.getb());
			break;
		}
		case SPC_Fade: {
			odaproto::Color* color = secmsg->mutable_colormap()->mutable_fade();
			color->set_r(sector.colormap->fade.getr());
			color->set_g(sector.colormap->fade.getg());
			color->set_b(sector.colormap->fade.getb());
			break;
		}
		case SPC_Gravity:
			secmsg->set_gravity(sector.gravity);
			break;
		case SPC_Panning: {
			odaproto::Vec2* mc = secmsg->mutable_ceiling_offs();
			mc->set_x(sector.ceiling_xoffs);
			mc->set_y(sector.ceiling_yoffs);

			odaproto::Vec2* mf = secmsg->mutable_floor_offs();
			mf->set_x(sector.floor_xoffs);
			mf->set_y(sector.floor_yoffs);
			break;
		}
		case SPC_Scale: {
			odaproto::Vec2* cs = secmsg->mutable_ceiling_scale();
			cs->set_x(sector.ceiling_xscale);
			cs->set_y(sector.ceiling_yscale);

			odaproto::Vec2* fs = secmsg->mutable_floor_scale();
			fs->set_x(sector.floor_xscale);
			fs->set_y(sector.floor_yscale);
			break;
		}
		case SPC_Rotation:
			secmsg->set_floor_angle(sector.floor_angle);
			secmsg->set_ceiling_angle(sector.ceiling_angle);
			break;
		case SPC_AlignBase:
			secmsg->set_base_ceiling_angle(sector.base_ceiling_angle);
			secmsg->set_base_ceiling_yoffs(sector.base_ceiling_yoffs);
			secmsg->set_base_floor_angle(sector.base_floor_angle);
			secmsg->set_base_floor_yoffs(sector.base_floor_yoffs);
			break;
		default:
			break;
		}
	}

	return msg;
}

odaproto::svc::LineSideUpdate SVC_LineSideUpdate(const line_t& line, const int sideNum)
{
	odaproto::svc::LineSideUpdate msg;

	side_t* currentSideDef = sides + line.sidenum[sideNum];

	msg.set_linenum(&line - ::lines);
	msg.set_side(sideNum);
	msg.set_changes(currentSideDef->SidedefChanges);

	for (int i = 0, prop = 1; prop < SDPC_Max; i++)
	{
		prop = BIT(i);
		if ((prop & currentSideDef->SidedefChanges) == 0)
			continue;

		switch (prop)
		{
		case SDPC_TexTop:
			msg.set_toptexture(currentSideDef->toptexture);
			break;
		case SDPC_TexMid:
			msg.set_midtexture(currentSideDef->midtexture);
			break;
		case SDPC_TexBottom:
			msg.set_bottomtexture(currentSideDef->bottomtexture);
			break;
		default:
			break;
		}
	}

	return msg;
}

odaproto::svc::MobjState SVC_MobjState(AActor* mo)
{
	odaproto::svc::MobjState msg;

	statenum_t mostate = static_cast<statenum_t>(mo->state - states);

	msg.set_netid(mo->netid);
	msg.set_mostate(mostate);

	return msg;
}

odaproto::svc::DamageMobj SVC_DamageMobj(AActor* target, const int pain)
{
	odaproto::svc::DamageMobj msg;

	msg.set_netid(target->netid);
	msg.set_health(target->health);
	msg.set_pain(pain);

	return msg;
}

odaproto::svc::ExecuteLineSpecial SVC_ExecuteLineSpecial(byte special, line_t* line,
                                                         AActor* mo, const int (&args)[5])
{
	odaproto::svc::ExecuteLineSpecial msg;

	msg.set_special(special);
	msg.set_linenum(line ? line - ::lines : -1);
	msg.set_activator_netid(mo ? mo->netid : 0);
	msg.set_arg0(args[0]);
	msg.set_arg1(args[1]);
	msg.set_arg2(args[2]);
	msg.set_arg3(args[3]);
	msg.set_arg4(args[4]);

	return msg;
}

/**
 * @brief Execute an ACS special on the client.
 *
 * @param special Special ID.
 * @param activator Activator pointer, NULL if no activator.
 * @param print String to send to client, NULL if no string.
 * @param args Arguments to pass to the client.
 */
odaproto::svc::ExecuteACSSpecial SVC_ExecuteACSSpecial(const byte special,
                                                       const AActor* activator,
                                                       const char* print,
                                                       const std::vector<int>& args)
{
	odaproto::svc::ExecuteACSSpecial msg;

	msg.set_special(special);
	if (activator != NULL)
	{
		msg.set_activator_netid(activator->netid);
	}
	if (print != NULL)
	{
		msg.set_print(print);
	}

	for (std::vector<int>::const_iterator it = args.begin(); it != args.end(); ++it)
	{
		msg.add_args(*it);
	}

	return msg;
}

odaproto::svc::ThinkerUpdate SVC_ThinkerUpdate(DThinker* thinker)
{
	odaproto::svc::ThinkerUpdate msg;

	if (thinker->IsA(RUNTIME_CLASS(DScroller)))
	{
		DScroller* scroller = static_cast<DScroller*>(thinker);
		odaproto::svc::ThinkerUpdate_Scroller* smsg = msg.mutable_scroller();
		smsg->set_type(scroller->GetType());
		smsg->set_scroll_x(scroller->GetScrollX());
		smsg->set_scroll_y(scroller->GetScrollY());
		smsg->set_affectee(scroller->GetAffectee());
		smsg->set_control(scroller->GetControl());
		smsg->set_accel(scroller->GetAccel());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DFireFlicker)))
	{
		DFireFlicker* fireFlicker = static_cast<DFireFlicker*>(thinker);
		odaproto::svc::ThinkerUpdate_FireFlicker* ffmsg = msg.mutable_fire_flicker();
		ffmsg->set_sector(fireFlicker->GetSector() - sectors);
		ffmsg->set_min_light(fireFlicker->GetMinLight());
		ffmsg->set_max_light(fireFlicker->GetMaxLight());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DFlicker)))
	{
		DFlicker* flicker = static_cast<DFlicker*>(thinker);
		odaproto::svc::ThinkerUpdate_Flicker* fmsg = msg.mutable_flicker();
		fmsg->set_sector(flicker->GetSector() - sectors);
		fmsg->set_min_light(flicker->GetMinLight());
		fmsg->set_max_light(flicker->GetMaxLight());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DLightFlash)))
	{
		DLightFlash* lightFlash = static_cast<DLightFlash*>(thinker);
		odaproto::svc::ThinkerUpdate_LightFlash* lfmsg = msg.mutable_light_flash();
		lfmsg->set_sector(lightFlash->GetSector() - sectors);
		lfmsg->set_min_light(lightFlash->GetMinLight());
		lfmsg->set_max_light(lightFlash->GetMaxLight());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DStrobe)))
	{
		DStrobe* strobe = static_cast<DStrobe*>(thinker);
		odaproto::svc::ThinkerUpdate_Strobe* smsg = msg.mutable_strobe();
		smsg->set_sector(strobe->GetSector() - sectors);
		smsg->set_min_light(strobe->GetMinLight());
		smsg->set_max_light(strobe->GetMaxLight());
		smsg->set_dark_time(strobe->GetDarkTime());
		smsg->set_bright_time(strobe->GetBrightTime());
		smsg->set_count(strobe->GetCount());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DGlow)))
	{
		DGlow* glow = static_cast<DGlow*>(thinker);
		odaproto::svc::ThinkerUpdate_Glow* gmsg = msg.mutable_glow();
		gmsg->set_sector(glow->GetSector() - sectors);
	}
	else if (thinker->IsA(RUNTIME_CLASS(DGlow2)))
	{
		DGlow2* glow2 = static_cast<DGlow2*>(thinker);
		odaproto::svc::ThinkerUpdate_Glow2* g2msg = msg.mutable_glow2();
		g2msg->set_sector(glow2->GetSector() - sectors);
		g2msg->set_start(glow2->GetStart());
		g2msg->set_end(glow2->GetEnd());
		g2msg->set_max_tics(glow2->GetMaxTics());
		g2msg->set_one_shot(glow2->GetOneShot());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DPhased)))
	{
		DPhased* phased = static_cast<DPhased*>(thinker);
		odaproto::svc::ThinkerUpdate_Phased* pmsg = msg.mutable_phased();
		pmsg->set_sector(phased->GetSector() - sectors);
		pmsg->set_base_level(phased->GetBaseLevel());
		pmsg->set_phase(phased->GetPhase());
	}

	return msg;
}

odaproto::svc::VoteUpdate SVC_VoteUpdate(const vote_state_t& state)
{
	odaproto::svc::VoteUpdate msg;

	msg.set_result(state.result);
	msg.set_votestring(state.votestring);
	msg.set_countdown(state.countdown);
	msg.set_yes(state.yes);
	msg.set_yes_needed(state.yes_needed);
	msg.set_no(state.no);
	msg.set_no_needed(state.no_needed);
	msg.set_abs(state.abs);

	return msg;
}

odaproto::svc::Maplist SVC_Maplist(const maplist_status_t status)
{
	odaproto::svc::Maplist msg;

	msg.set_status(status);

	return msg;
}

odaproto::svc::MaplistUpdate SVC_MaplistUpdate(const maplist_status_t status,
                                               const maplist_qrows_t* maplist)
{
	odaproto::svc::MaplistUpdate msg;
	msg.set_status(status);

	if (status == MAPLIST_OUTDATED)
	{
		assert(maplist != NULL && "Maplist pointer must be non-null");

		// In order to cut down on bandwidth, this message uses mappings of
		// strings to unique ID's to save bandwidth.  Some of these mappings
		// are already known on the receiving end.
		OStringIndexer indexer = OStringIndexer::maplistFactory();

		for (maplist_qrows_t::const_iterator it = maplist->begin(); it != maplist->end();
		     ++it)
		{
			// Create a row and add an indexed map to it.
			odaproto::svc::MaplistUpdate::Row* row = msg.add_maplist();
			const std::string& map = it->second->map;
			const uint32_t mapidx = indexer.getIndex(map);
			row->set_map(mapidx);

			for (std::vector<std::string>::iterator itr = it->second->wads.begin();
			     itr != it->second->wads.end(); ++itr)
			{
				// Push an indexed WAD into the message.
				std::string filename = D_CleanseFileName(*itr);
				const uint32_t wadidx = indexer.getIndex(filename);
				row->add_wads(wadidx);
			}
		}

		// Populate the dictionary.
		for (OStringIndexer::Indexes::const_iterator it = indexer.indexes.begin();
		     it != indexer.indexes.end(); ++it)
		{
			if (!indexer.shouldTransmit(it->second))
				continue;

			typedef google::protobuf::MapPair<uint32_t, std::string> DictPair;
			msg.mutable_dict()->insert(DictPair(it->second, it->first));
		}
	}

	return msg;
}

odaproto::svc::MaplistIndex SVC_MaplistIndex(const byte count, const size_t this_index,
                                             const size_t next_index)
{
	odaproto::svc::MaplistIndex msg;

	msg.set_count(count);
	if (count > 0)
	{
		msg.set_next_index(next_index);
		if (count > 1)
		{
			msg.set_this_index(this_index);
		}
	}

	return msg;
}

odaproto::svc::Toast SVC_Toast(const toast_t& toast)
{
	odaproto::svc::Toast msg;

	msg.set_flags(toast.flags);
	msg.set_left(toast.left);
	msg.set_left_pid(toast.left_pid);
	msg.set_right(toast.right);
	msg.set_right_pid(toast.right_pid);
	msg.set_icon(toast.icon);

	return msg;
}

odaproto::svc::HordeInfo SVC_HordeInfo(const hordeInfo_t& horde)
{
	odaproto::svc::HordeInfo msg;

	msg.set_state(horde.state);
	msg.set_wave(horde.wave);
	msg.set_wave_time(horde.waveTime);
	msg.set_boss_time(horde.bossTime);
	msg.set_define_id(horde.defineID);
	msg.set_legacy_id(horde.legacyID);
	msg.set_spawned_health(horde.spawnedHealth);
	msg.set_killed_health(horde.killedHealth);
	msg.set_boss_health(horde.bossHealth);
	msg.set_boss_damage(horde.bossDamage);
	msg.set_wave_start_health(horde.waveStartHealth);

	return msg;
}

odaproto::svc::NetdemoCap SVC_NetdemoCap(player_t* player)
{
	odaproto::svc::NetdemoCap msg;

	odaproto::Actor* act = msg.mutable_actor();
	odaproto::Player* play = msg.mutable_player();

	AActor* mo = player->mo;

	player->cmd.serialize(*msg.mutable_player_cmd());

	act->set_waterlevel(mo->waterlevel);
	act->mutable_pos()->set_x(mo->x);
	act->mutable_pos()->set_y(mo->y);
	act->mutable_pos()->set_z(mo->z);
	act->mutable_mom()->set_x(mo->momx);
	act->mutable_mom()->set_y(mo->momy);
	act->mutable_mom()->set_z(mo->momz);
	act->set_angle(mo->angle);
	act->set_pitch(mo->pitch);
	play->set_viewz(player->viewz);
	play->set_viewheight(player->viewheight);
	play->set_deltaviewheight(player->deltaviewheight);
	play->set_jumptics(player->jumpTics);
	act->set_reactiontime(mo->reactiontime);
	play->set_readyweapon(player->readyweapon);
	play->set_pendingweapon(player->pendingweapon);

	return msg;
}

VERSION_CONTROL(svc_message, "$Id$")
