#include "clc_message.h"

#include "odamex.h"

#include "d_player.h"

void CLC_ClientCommandFromPlayer(odaproto::ClientCommand& msg, const player_t& player)
{
	if (player.mo)
	{
		msg.Clear();
		msg.set_tic(player.cmd.tic);

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

void CLC_ClientCommandToPlayer(player_t& player, const odaproto::ClientCommand& msg)
{
	if (player.mo)
	{
		player.cmd.clear();
		player.cmd.tic = msg.tic();

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
