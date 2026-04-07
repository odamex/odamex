#include "clc_message.h"

#include "odamex.h"

#include "d_player.h"

void CLC_PackPlayerInputMessageFromPlayer(odaproto::clc::PlayerInput& msg, const player_t& player, int clientTic, int clientWorldIndex)
{
	if (player.mo)
	{
		msg.Clear();
		msg.set_tic(clientTic);
		msg.set_world_index(clientWorldIndex);

		// Special knowledge: On the client side, we save+send the command before we process it locally.
		//                    Once processed, we locally tic forward.  During that time, we may detect
		//                    conditions for which we set the Inventory Check Request, which means that
		//                    we want to send the request on the following tic.
		if (player.inventoryCheckIsRequestedForTic == clientTic - 1 and clientTic > 0)
		{
			msg.set_inventory_check_tic(player.inventoryCheckIsRequestedForTic);
		}

		if (player.cmd.buttons & BT_ATTACK)
		{
			msg.set_button_attack(true);
		}
		if (player.cmd.buttons & BT_USE)
		{
			msg.set_button_use(true);
		}
		if (player.cmd.buttons & BT_SPECIAL)
		{
			if (player.cmd.buttons & BTS_PAUSE)
			{
				msg.set_button_pause(true);
			}
			if (player.cmd.buttons & BTS_SAVEGAME)
			{
				msg.set_button_savegame((player.cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT);
			}
		}
		if (player.cmd.buttons & BT_CHANGE)
		{
			msg.set_button_weaponchange((player.cmd.buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT);
		}
		if (player.cmd.buttons & BT_JUMP)
		{
			msg.set_button_jump(true);
		}

		if (player.cmd.impulse)
		{
			msg.set_impulse(player.cmd.impulse);
		}

		if (player.playerstate != PST_DEAD)
		{
			msg.set_angle(player.mo->angle);
			msg.set_pitch(player.mo->pitch);

			if (player.cmd.forwardmove)
			{
				msg.set_move_forward(player.cmd.forwardmove);
			}
			if (player.cmd.sidemove)
			{
				msg.set_move_side(player.cmd.sidemove);
			}
			if (player.cmd.upmove)
			{
				msg.set_move_up(player.cmd.upmove);
			}
			if (player.cmd.yaw)
			{
				msg.set_delta_yaw(player.cmd.yaw);
			}
			if (player.cmd.pitch)
			{
				msg.set_delta_pitch(player.cmd.pitch);
			}
		}
	}
}

void CLC_UnpackPlayerInputMessageToPlayer(const odaproto::clc::PlayerInput& msg, player_t& player)
{
	// We deliberately avoid unpacking the player tic here.  That should be done carefully at the
	// callers' discretion for their particular use cases.
	if (player.mo)
	{
		player.cmd.clear();

		if (msg.button_attack())
		{
			player.cmd.buttons |= BT_ATTACK;
		}
		if (msg.button_use())
		{
			player.cmd.buttons |= BT_USE;
		}
		if (msg.button_pause())
		{
			player.cmd.buttons |= (BT_SPECIAL | BTS_PAUSE);
		}
		if (msg.has_button_savegame())
		{
			player.cmd.buttons |= (BT_SPECIAL | BTS_SAVEGAME | ((msg.button_savegame() << BTS_SAVESHIFT) & BTS_SAVEMASK));
		}
		if (msg.has_button_weaponchange())
		{
			player.cmd.buttons |= (BT_CHANGE | ((msg.button_weaponchange() << BT_WEAPONSHIFT) & BT_WEAPONMASK));
		}
		if (msg.button_jump())
		{
			player.cmd.buttons |= BT_JUMP;
		}

		player.cmd.impulse = msg.impulse();

		if (player.playerstate != PST_DEAD)
		{
			player.cmd.forwardmove  = msg.move_forward();
			player.cmd.sidemove     = msg.move_side();
			player.cmd.upmove       = msg.move_up();
			player.cmd.yaw          = msg.delta_yaw();
			player.cmd.pitch        = msg.delta_pitch();

			player.mo->angle = msg.angle();
			player.mo->pitch = msg.pitch();
		}
	}
}

odaproto::clc::NetdemoCap CLC_NetdemoCap(const player_t& player, const odaproto::clc::PlayerInput& inputMessage)
{
	odaproto::clc::NetdemoCap msg;

	odaproto::Actor* act = msg.mutable_actor();
	odaproto::Player* play = msg.mutable_player();

	const AActor* mo = player.mo;

	inputMessage.SerializeToString(msg.mutable_packed_player_input());

	act->set_waterlevel(mo->waterlevel);
	act->mutable_pos()->set_x(mo->x);
	act->mutable_pos()->set_y(mo->y);
	act->mutable_pos()->set_z(mo->z);
	act->mutable_mom()->set_x(mo->momx);
	act->mutable_mom()->set_y(mo->momy);
	act->mutable_mom()->set_z(mo->momz);
	act->set_angle(mo->angle);
	act->set_pitch(mo->pitch);
	act->set_reactiontime(mo->reactiontime);

	play->set_viewz(player.viewz);
	play->set_viewheight(player.viewheight);
	play->set_deltaviewheight(player.deltaviewheight);
	play->set_jumptics(player.jumpTics);

	play->mutable_inventory()->set_readyweapon(player.readyweapon);
	play->mutable_inventory()->set_pendingweapon(player.pendingweapon);

	return msg;
}

