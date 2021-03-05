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

#include <bitset>

#include "svc_message.h"

#include "d_main.h"
#include "i_system.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "p_unlag.h"
#include "common.pb.h"
#include "server.pb.h"

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

void SVC_Disconnect(buf_t& b, const char* message)
{
	odaproto::svc::Disconnect msg;
	if (message != NULL)
	{
		msg.set_message(message);
	}

	MSG_WriteMarker(&b, svc_disconnect);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about a player.
 */
void SVC_PlayerInfo(buf_t& b, player_t& player)
{
	odaproto::svc::PlayerInfo msg;

	uint32_t packedweapons = PackBoolArray(player.weaponowned, NUMWEAPONS);
	msg.mutable_player()->set_weaponowned(packedweapons);

	uint32_t packedcards = PackBoolArray(player.cards, NUMCARDS);
	msg.mutable_player()->set_cards(packedweapons);

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

	MSG_WriteMarker(&b, svc_playerinfo);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Change the location of a player.
 */
void SVC_MovePlayer(buf_t& b, player_t& player, const int tic)
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

	MSG_WriteMarker(&b, svc_moveplayer);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send the local player position for a client.
 */
void SVC_UpdateLocalPlayer(buf_t& b, AActor& mo, const int tic)
{
	// client player will update his position if packets were missed
	odaproto::svc::UpdateLocalPlayer msg;
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

	MSG_WriteMarker(&b, svc_updatelocalplayer);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Persist level locals to the client.
 *
 * @param b Buffer to write to.
 * @param locals Level locals struct to send.
 * @param flags SVC_LL_* bit flags to designate what gets sent.
 */
void SVC_LevelLocals(buf_t& b, const level_locals_t& locals, uint32_t flags)
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

	MSG_WriteMarker(&b, svc_levellocals);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send the server's current time in MS to the client.
 */
void SVC_PingRequest(buf_t& b)
{
	odaproto::svc::PingRequest msg;

	msg.set_ms_time(I_MSTime());

	MSG_WriteMarker(&b, svc_pingrequest);
	MSG_WriteProto(&b, msg);
}

void SVC_UpdatePing(buf_t& b, player_t& player)
{
	odaproto::svc::UpdatePing msg;

	msg.set_pid(player.id);
	msg.set_ping(player.ping);

	MSG_WriteMarker(&b, svc_updateping);
	MSG_WriteProto(&b, msg);
}

void SVC_SpawnMobj(buf_t& b, AActor* mo)
{
	odaproto::svc::SpawnMobj msg;
	odaproto::Actor* actor = msg.mutable_actor();
	odaproto::Vec3* pos = actor->mutable_pos();

	uint32_t flags = 0;

	pos->set_x(mo->x);
	pos->set_y(mo->y);
	pos->set_z(mo->z);

	actor->set_angle(mo->angle);

	actor->set_type(mo->type);
	actor->set_netid(mo->netid);
	actor->set_rndindex(mo->rndindex);
	actor->set_statenum(mo->state - states); // denis - sending state fixes monster ghosts appearing under doors

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
		flags |= SVC_SM_MISSILE;
		msg.set_target_netid(mo->target ? mo->target->netid : 0);
		actor->set_angle(mo->angle);
		actor->mutable_mom()->set_x(mo->momx);
		actor->mutable_mom()->set_y(mo->momy);
		actor->mutable_mom()->set_z(mo->momz);
	}
	else if (mo->flags & MF_AMBUSH || mo->flags & MF_DROPPED)
	{
		flags |= SVC_SM_FLAGS;
		actor->set_flags(mo->flags);
	}

	// animating corpses
	if ((mo->flags & MF_CORPSE) && mo->state - states != S_GIBS)
	{
		// This sets off some additional logic on the client.
		flags |= SVC_SM_CORPSE;
		actor->set_frame(mo->frame);
		actor->set_tics(mo->tics);
	}

	msg.set_flags(flags);

	MSG_WriteMarker(&b, svc_spawnmobj);
	MSG_WriteProto(&b, msg);
}

void SVC_DisconnectClient(buf_t& b, player_t& player)
{
	odaproto::svc::DisconnectClient msg;

	msg.set_pid(player.id);

	MSG_WriteMarker(&b, svc_disconnectclient);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Sends a message to a player telling them to change to the specified
 *        WAD and DEH patch files and load a map.
 */
void SVC_LoadMap(buf_t& b, const OResFiles& wadnames, const OResFiles& patchnames,
                 const std::string& mapname, int time)
{
	odaproto::svc::LoadMap msg;

	// send list of wads (skip over wadnames[0] == odamex.wad)
	size_t wadcount = wadnames.size() - 1;
	for (size_t i = 1; i < wadcount + 1; i++)
	{
		odaproto::svc::LoadMap_Resource* wad = msg.add_wadnames();
		wad->set_name(wadnames[i].getBasename());
		wad->set_hash(wadnames[i].getHash());
	}

	// send list of DEH/BEX patches
	size_t patchcount = patchnames.size();
	for (size_t i = 0; i < patchcount; i++)
	{
		odaproto::svc::LoadMap_Resource* patch = msg.add_patchnames();
		patch->set_name(patchnames[i].getBasename());
		patch->set_hash(patchnames[i].getHash());
	}

	msg.set_mapname(mapname);
	msg.set_time(time);

	MSG_WriteMarker(&b, svc_loadmap);
	MSG_WriteProto(&b, msg);
}

void SVC_ConsolePlayer(buf_t& b, player_t& player, const std::string& digest)
{
	odaproto::svc::ConsolePlayer msg;

	msg.set_pid(player.id);
	msg.set_digest(digest);

	MSG_WriteMarker(&b, svc_consoleplayer);
	MSG_WriteProto(&b, msg);
}

void SVC_ExplodeMissile(buf_t& b, AActor& mobj)
{
	odaproto::svc::ExplodeMissile msg;

	msg.set_netid(mobj.netid);

	MSG_WriteMarker(&b, svc_explodemissile);
	MSG_WriteProto(&b, msg);
}

void SVC_RemoveMobj(buf_t& b, AActor& mobj)
{
	odaproto::svc::RemoveMobj msg;

	msg.set_netid(mobj.netid);

	MSG_WriteMarker(&b, svc_removemobj);
	MSG_WriteProto(&b, msg);
}

void SVC_UserInfo(buf_t& b, player_t& player, int64_t time)
{
	odaproto::svc::UserInfo msg;

	msg.set_pid(player.id);
	msg.set_netname(player.userinfo.netname);
	msg.set_team(player.userinfo.team);
	msg.set_gender(player.userinfo.gender);

	for (size_t i = 0; i < ARRAY_LENGTH(player.userinfo.color); i++)
	{
		msg.mutable_color()->Add(player.userinfo.color[i]);
	}

	msg.set_join_time(time);

	MSG_WriteMarker(&b, svc_userinfo);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Move a mobj to a new location.  If it's a player, it will update
 *        the client's snapshot.
 */
void SVC_UpdateMobj(buf_t& b, AActor& mobj, uint32_t flags)
{
	odaproto::svc::UpdateMobj msg;
	odaproto::Actor* act = msg.mutable_actor();

	msg.set_flags(flags);

	if (flags & SVC_UM_POS_RND)
	{
		odaproto::Vec3* pos = act->mutable_pos();
		pos->set_x(mobj.x);
		pos->set_y(mobj.y);
		pos->set_z(mobj.z);
		act->set_rndindex(mobj.rndindex);
	}

	if (flags & SVC_UM_MOM_ANGLE)
	{
		odaproto::Vec3* mom = act->mutable_mom();
		mom->set_x(mobj.momx);
		mom->set_y(mobj.momy);
		mom->set_z(mobj.momz);
		act->set_angle(mobj.angle);
	}

	if (flags & SVC_UM_MOVEDIR)
	{
		act->set_movedir(mobj.movedir);
		act->set_movecount(mobj.movecount);
	}

	if (flags & SVC_UM_TARGET)
	{
		act->set_targetid(mobj.target ? mobj.target->netid : 0);
	}

	if (flags & SVC_UM_TRACER)
	{
		act->set_tracerid(mobj.target ? mobj.target->netid : 0);
	}

	MSG_WriteMarker(&b, svc_updatemobj);
	MSG_WriteProto(&b, msg);
}

void SVC_SpawnPlayer(buf_t& b, player_t& player)
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

	MSG_WriteMarker(&b, svc_spawnplayer);
	MSG_WriteProto(&b, msg);
}

void SVC_DamagePlayer(buf_t& b, player_t& player, int health, int armor)
{
	odaproto::svc::DamagePlayer msg;

	msg.set_netid(player.mo->netid);
	msg.set_health_damage(health);
	msg.set_armor_damage(armor);

	MSG_WriteMarker(&b, svc_damageplayer);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Kill a mobj.
 */
void SVC_KillMobj(buf_t& b, AActor* source, AActor* target, AActor* inflictor, int mod,
                  bool joinkill)
{
	odaproto::svc::KillMobj msg;
	odaproto::Actor* tgt = msg.mutable_target();

	if (source)
	{
		msg.set_source_netid(source->netid);
	}
	else
	{
		msg.set_source_netid(0);
	}

	msg.set_inflictor_netid(inflictor ? inflictor->netid : 0);
	msg.set_health(target->health);
	msg.set_mod(mod);
	msg.set_joinkill(joinkill);

	// [AM] Confusingly, we send the lives _before_ we take it away, so
	//      the lives logic can live in the kill function.
	if (target->player)
	{
		msg.set_lives(target->player->lives);
	}
	else
	{
		msg.set_lives(-1);
	}

	// send death location first
	tgt->set_netid(target->netid);
	tgt->set_rndindex(target->rndindex);

	// [SL] 2012-12-26 - Get real position since this actor is at
	// a reconciled position with sv_unlag 1
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

	MSG_WriteMarker(&b, svc_killmobj);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Persist player information to the client that is less vital.
 *
 * @param b Buffer to write to.
 * @param player Player to send information about.
 * @param flags SVC_PM_* flags to designate what gets sent.
 */
void SVC_PlayerMembers(buf_t& b, player_t& player, byte flags)
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

	MSG_WriteMarker(&b, svc_playermembers);
	MSG_WriteProto(&b, msg);
}

/**
 * Persist mutable team information to the client.
 */
void SVC_TeamMembers(buf_t& b, team_t team)
{
	odaproto::svc::TeamMembers msg;

	TeamInfo* info = GetTeamInfo(team);

	msg.set_team(team);
	msg.set_points(info->Points);
	msg.set_roundwins(info->RoundWins);

	MSG_WriteMarker(&b, svc_teammembers);
	MSG_WriteProto(&b, msg);
}

void SVC_ActivateLine(buf_t& b, line_t* line, AActor* mo, int side,
                      LineActivationType type)
{
	odaproto::svc::ActivateLine msg;

	msg.set_linenum(line ? line - ::lines : -1);
	msg.set_activator_netid(mo ? mo->netid : 0);
	msg.set_side(side);
	msg.set_activation_type(type);

	MSG_WriteMarker(&b, svc_activateline);
	MSG_WriteProto(&b, msg);
}

void SVC_MovingSector(buf_t& b, const sector_t& sector)
{
	odaproto::svc::MovingSector msg;

	ptrdiff_t sectornum = &sector - sectors;

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
		return;
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

	MSG_WriteMarker(&b, svc_movingsector);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about a player
 */
void SVC_PlayerState(buf_t& b, player_t& player)
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

	MSG_WriteMarker(&b, svc_playerstate);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about the server's LevelState to the client.
 */
void SVC_LevelState(buf_t& b, const SerializedLevelState& sls)
{
	odaproto::svc::LevelState msg;

	msg.set_state(sls.state);
	msg.set_countdown_done_time(sls.countdown_done_time);
	msg.set_ingame_start_time(sls.ingame_start_time);
	msg.set_round_number(sls.round_number);
	msg.set_last_wininfo_type(sls.last_wininfo_type);
	msg.set_last_wininfo_id(sls.last_wininfo_id);

	MSG_WriteMarker(&b, svc_levelstate);
	MSG_WriteProto(&b, msg);
}

/**
 * @brief Send information about a player who discovered a secret.
 */
void SVC_SecretFound(buf_t& b, int playerid, int sectornum)
{
	sector_t* sector = &sectors[sectornum];

	// Only update secret sectors that've been discovered.
	// [AM} FIXME: This function should not contain this kind of logic.
	if (sector != NULL && (!(sector->special & SECRET_MASK) && sector->secretsector))
	{
		MSG_WriteMarker(&b, svc_secretevent);
		MSG_WriteByte(&b, playerid); // ID of player who discovered it
		MSG_WriteShort(&b, sectornum);
		MSG_WriteShort(&b, sector->special);
	}
}

void SVC_SectorProperties(buf_t& b, sector_t& sector)
{
	odaproto::svc::SectorProperties msg;

	msg.set_sector(&sector - ::sectors);
	msg.set_changes(sector.SectorChanges);

	for (int i = 0, prop = 1; prop < SPC_Max; i++)
	{
		prop = 1 << i;
		if ((prop & sector.SectorChanges) == 0)
			continue;

		switch (prop)
		{
		case SPC_FlatPic:
			msg.set_floorpic(sector.floorpic);
			msg.set_ceilingpic(sector.ceilingpic);
			break;
		case SPC_LightLevel:
			msg.set_lightlevel(sector.lightlevel);
			break;
		case SPC_Color: {
			odaproto::Color* color = msg.mutable_color();
			color->set_r(sector.colormap->color.getr());
			color->set_g(sector.colormap->color.getg());
			color->set_b(sector.colormap->color.getb());
			break;
		}
		case SPC_Fade: {
			odaproto::Color* color = msg.mutable_fade();
			color->set_r(sector.colormap->fade.getr());
			color->set_g(sector.colormap->fade.getg());
			color->set_b(sector.colormap->fade.getb());
			break;
		}
		case SPC_Gravity:
			msg.set_gravity(sector.gravity);
			break;
		case SPC_Panning: {
			odaproto::Vec2* mc = msg.mutable_ceiling_offs();
			mc->set_x(sector.ceiling_xoffs);
			mc->set_y(sector.ceiling_yoffs);

			odaproto::Vec2* mf = msg.mutable_floor_offs();
			mf->set_x(sector.floor_xoffs);
			mf->set_y(sector.floor_yoffs);
			break;
		}
		case SPC_Scale: {
			odaproto::Vec2* cs = msg.mutable_ceiling_scale();
			cs->set_x(sector.ceiling_xscale);
			cs->set_y(sector.ceiling_yscale);

			odaproto::Vec2* fs = msg.mutable_floor_scale();
			fs->set_x(sector.floor_xscale);
			fs->set_y(sector.floor_yscale);
			break;
		}
		case SPC_Rotation:
			msg.set_floor_angle(sector.floor_angle);
			msg.set_ceiling_angle(sector.ceiling_angle);
			break;
		case SPC_AlignBase:
			msg.set_base_ceiling_angle(sector.base_ceiling_angle);
			msg.mutable_base_ceiling_offs()->set_y(sector.base_ceiling_yoffs);
			msg.set_base_floor_angle(sector.base_floor_angle);
			msg.mutable_base_floor_offs()->set_y(sector.base_floor_yoffs);
			break;
		default:
			break;
		}
	}

	MSG_WriteMarker(&b, svc_sectorproperties);
	MSG_WriteProto(&b, msg);
}

void SVC_ExecuteLineSpecial(buf_t& b, byte special, line_t* line, AActor* mo,
                            const int (&args)[5])
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

	MSG_WriteMarker(&b, svc_executelinespecial);
	MSG_WriteProto(&b, msg);
}

void SVC_ThinkerUpdate(buf_t& b, DThinker* thinker)
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
		odaproto::svc::ThinkerUpdate_Glow2* gmsg = msg.mutable_glow2();
		gmsg->set_sector(glow2->GetSector() - sectors);
		gmsg->set_start(glow2->GetStart());
		gmsg->set_end(glow2->GetEnd());
		gmsg->set_max_tics(glow2->GetMaxTics());
		gmsg->set_one_shot(glow2->GetOneShot());
	}
	else if (thinker->IsA(RUNTIME_CLASS(DPhased)))
	{
		DPhased* phased = static_cast<DPhased*>(thinker);
		odaproto::svc::ThinkerUpdate_Phased* pmsg = msg.mutable_phased();
		pmsg->set_sector(phased->GetSector() - sectors);
		pmsg->set_base_level(phased->GetBaseLevel());
		pmsg->set_phase(phased->GetPhase());
	}

	MSG_WriteMarker(&b, svc_thinkerupdate);
	MSG_WriteProto(&b, msg);
}

void SVC_NetdemoCap(buf_t& b, player_t* player)
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

	MSG_WriteByte(&b, svc_netdemocap);
	MSG_WriteProto(&b, msg);
}

VERSION_CONTROL(svc_message, "$Id$")
