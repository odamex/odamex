//
// OdaTV - Allow multiple clients to watch the same server
// without creating extra traffic on that server
//
// This proxy will create the first connection transparently,
// but other connections will be hidden from the server
//

#include <string.h>
#include <vector>
#include <iostream>

#include "../../master/i_net.h"

std::vector<netadr_t> spectators;
netadr_t net_server;

buf_t challenge_message(MAX_UDP_PACKET), first_message(MAX_UDP_PACKET);

class MessageTranslator
{
protected:
	void Copy(buf_t &in, buf_t &out, int len)
	{
		byte *p = in.ReadChunk(len);
		if(p)
			out.WriteChunk((const char *)p, len);
	}

	void CopyString(buf_t &in, buf_t &out)
	{
		const char *p = in.ReadString();
		if(p)
			out.WriteString(p);
	}
};

class Translate_ServerToTV : public MessageTranslator
{
public:
// network messages
enum svc_t
{
	svc_abort,
	svc_full,
	svc_disconnect,
	svc_reserved3,
	svc_playerinfo,			// weapons, ammo, maxammo, raisedweapon for local player
	svc_moveplayer,			// [byte] [int] [int] [int] [int] [byte]
	svc_updatelocalplayer,	// [int] [int] [int] [int] [int]
	svc_svgametic,			// [int]
	svc_updateping,			// [byte] [byte]
	svc_spawnmobj,			//
	svc_disconnectclient,
	svc_loadmap,
	svc_consoleplayer,
	svc_mobjspeedangle,
	svc_explodemissile,		// [short] - netid
	svc_removemobj,
	svc_userinfo,
	svc_movemobj,			// [short] [byte] [int] [int] [int]
	svc_spawnplayer,
	svc_damageplayer,
	svc_killmobj,
	svc_firepistol,			// [byte] - playernum
	svc_fireshotgun,		// [byte] - playernum
	svc_firessg,			// [byte] - playernum
	svc_firechaingun,		// [byte] - playernum
	svc_fireweapon,			// [byte]
	svc_sector,
	svc_print,
	svc_mobjinfo,
	svc_updatefrags,		// [byte] [short]
	svc_teampoints,
	svc_activateline,
	svc_movingsector,
	svc_startsound,
	svc_reconnect,
	svc_exitlevel,
	svc_touchspecial,
	svc_changeweapon,
	svc_reserved42,
	svc_corpse,
	svc_missedpacket,
	svc_soundorigin,
	svc_reserved46,
	svc_reserved47,
	svc_forceteam,			// [Toke] Allows server to change a clients team setting.
	svc_switch,
	svc_reserved50,
	svc_reserved51,
	svc_spawnhiddenplayer,	// [denis] when client can't see player
	svc_updatedeaths,		// [byte] [short]
	svc_ctfevent,			// [Toke - CTF] - [int]
	svc_serversettings,		// 55 [Toke] - informs clients of server settings
	svc_spectate,			// [Nes] - [byte:state], [short:playernum]

	// for co-op
	svc_mobjstate = 70,
	svc_actor_movedir,
	svc_actor_target,
	svc_actor_tracer,
	svc_damagemobj,

	// for downloading
	svc_wadinfo,			// denis - [ulong:filesize]
	svc_wadchunk,			// denis - [ulong:offset], [ushort:len], [byte[]:data]
	
	// for compressed packets
	svc_compressed = 200,

	// for when launcher packets go astray
	svc_launcher_challenge = 212,
	svc_challenge = 163,
	svc_max = 255
};

public:

	std::string map, digest;
	byte consoleplayer;
	int playermobj;
	int spawnpos[3];
	unsigned int spawnang;

	void Go(buf_t &in, buf_t &out)
	{
		while(in.BytesLeftToRead())
		{
			byte cmd = in.NextByte();
//std::cout << cmd << " " << (int)cmd << std::endl;
			switch(cmd)
			{
				case svc_loadmap:
					{
						Copy(in, out, 1);
						map = in.ReadString();
						out.WriteString(map.c_str());
						std::cout<< "map " << map << std::endl; // todo unsafe
					}
					break;
				case svc_playerinfo:
					Copy(in, out, 1+ 9 + 4*4 + 5);
					break;
				case svc_consoleplayer:
					Copy(in, out, 1);
					consoleplayer = in.ReadByte();
					out.WriteByte(consoleplayer);
					digest = in.ReadString();
					out.WriteString(digest.c_str());
					break;
				case svc_updatefrags:
					Copy(in, out, 1+ 7);
					break;
				case svc_moveplayer:
					Copy(in, out, 1+ 37);
					break;
				case svc_updatelocalplayer:
					//in.ReadChunk(28);
					Copy(in, out, 1+ 28);
					break;
				case svc_userinfo:
					Copy(in, out, 1+ 1);
					CopyString(in, out);
					Copy(in, out, 9);
					CopyString(in, out);
					Copy(in, out, 2);
					break;
				case svc_teampoints:
					Copy(in, out, 1+ 3*2);
					break;
				case svc_svgametic:
					Copy(in, out, 1+ 4);
					break;
				case svc_updateping:
					Copy(in, out, 1+ 5);
					break;
				case svc_spawnmobj:
					{
						Copy(in, out, 1+ 16);
						unsigned short type = in.ReadShort();
						out.WriteShort(type); // todo: if this line is missing, odamex client crashes
						Copy(in, out, 5);
						if(type == 0x10000/*MF_MISSILE*/)
						{
							out.WriteShort(type);
							Copy(in, out, 16); //SpeedAndAngle
						}
					}
					break;
				case svc_mobjspeedangle:
					Copy(in, out, 1+ 18);
					break;
				case svc_mobjinfo:
					Copy(in, out, 1+ 6);
					break;
				case svc_explodemissile:
					Copy(in, out, 1+ 2);
					break;
				case svc_removemobj:
					Copy(in, out, 1+ 2);
					break;
				case svc_killmobj:
					Copy(in, out, 1+ 13);
					break;
				case svc_movemobj:
					Copy(in, out, 1+ 15);
					break;
				case svc_damagemobj:
					Copy(in, out, 1+ 5);
					break;
				case svc_corpse:
					Copy(in, out, 1+ 4);
					break;
				case svc_spawnplayer:
					{
						Copy(in, out, 1);
						byte player = in.ReadByte();
						out.WriteByte(player);
						unsigned short netid = in.ReadShort();
						if(player == consoleplayer)
							playermobj = netid;
						out.WriteShort(netid);
						spawnang = in.ReadLong();
						out.WriteLong(spawnang);
						spawnpos[0] = in.ReadLong();
						out.WriteLong(spawnpos[0]);
						spawnpos[1] = in.ReadLong();
						out.WriteLong(spawnpos[1]);
						spawnpos[2] = in.ReadLong();
						out.WriteLong(spawnpos[2]);
					}
					break;
				case svc_damageplayer:
					Copy(in, out, 1+ 4);
					break;
				case svc_firepistol:
				case svc_fireshotgun:
				case svc_firessg:
				case svc_firechaingun:
					Copy(in, out, 1+ 1);
					break;
				case svc_disconnectclient:
					Copy(in, out, 1+ 1);
					break;
				case svc_activateline:
					Copy(in, out, 1+ 8);
					break;
				case svc_sector:
					Copy(in, out, 1+ 10);
					break;
				case svc_movingsector:
					Copy(in, out, 1+ 19);
					break;
				case svc_switch:
					Copy(in, out, 1+ 10);
					break;
				case svc_print:
					Copy(in, out, 1+ 1);
					CopyString(in ,out);
					break;
				case svc_startsound:
					Copy(in, out, 1+ 14);
					break;
				case svc_soundorigin:
					Copy(in, out, 1+ 12);
					break;
				case svc_mobjstate:
					Copy(in, out, 1+ 4);
					break;
				case svc_actor_movedir:
					Copy(in, out, 1+ 7);
					break;
				case svc_actor_target:
				case svc_actor_tracer:
					Copy(in, out, 1+ 4);
					break;
				case svc_missedpacket:
					Copy(in, out, 1+ 6);
					break;
				case svc_forceteam:
					Copy(in, out, 1+ 2);
					break;
				case svc_ctfevent:
					{
						byte event = in.ReadByte();
					}
					break;
				case svc_serversettings:
					{
						Copy(in, out, 1);;
						while(true)
						{
							byte type = in.ReadByte();
							out.WriteByte(type);
							if(type == 1)
							{
								CopyString(in, out);
								CopyString(in, out);
							}
							else break;
						}
					}
					break;
				case svc_disconnect:
					Copy(in, out, 1);
					break;
				case svc_full:
					Copy(in, out, 1);
					break;
				case svc_reconnect:
					Copy(in, out, 1);
					break;
				case svc_exitlevel:
					Copy(in, out, 1);
					break;
				case svc_wadinfo:
					in.ReadChunk(1+ 4);
					break;
				case svc_wadchunk:
					in.ReadChunk(1+ 4);
					break;
				case svc_challenge:
					Copy(in, out, 1);
					break;
				case svc_launcher_challenge:
					Copy(in, out, 1);
					break;
				case svc_spectate:
					Copy(in, out, 1+ 2);
					break;
				case svc_abort:
				default:
					std::cout << "abort" << (int)cmd << std::endl;
					in.ReadByte();
					break;
			}
			if(in.overflowed)
			{
				std::cout << "overflowed on cmd " << (int)cmd << std::endl;
			}
		}
	}
};

void OnInitTV()
{
	NET_StringToAdr("voxelsoft.com:10666", &net_server);
	NET_StringToAdr("127.0.0.1:10666", &net_server);	
}

void OnClientTV(int i)
{
	if(i == 0)
	{
		// only forward the first client's packet
		NET_SendPacket(net_message.cursize, net_message.data, net_server);
	}
	else	
	{
		// TODO should keep client's tick info
		// TODO and also catch client disconnect messages
	}
}

Translate_ServerToTV tr;

void OnNewClientTV()
{
	spectators.push_back(net_from);

	if(spectators.size() == 1)
	{
		// only forward the first client's packets
		NET_SendPacket(net_message.cursize, net_message.data, net_server);

		// pass messages through the translator so it discovers maps, etc
		buf_t tmp;
		net_message.ReadLong();
		tr.Go(net_message, tmp);

		std::cout << "first client connect from " << NET_AdrToString(net_from) << std::endl;
	}
	else
	{
		// send back to the non-first client a fake challenge
		NET_SendPacket(challenge_message.cursize, challenge_message.data, net_from);

		// to the first message, add map load command
		buf_t out(8196);// = first_message;
		out.WriteLong(0);
		out.WriteByte(Translate_ServerToTV::svc_consoleplayer);
		out.WriteByte(tr.consoleplayer);
		out.WriteString(tr.digest.c_str());

		out.WriteByte(Translate_ServerToTV::svc_loadmap);
		out.WriteString(tr.map.c_str());

		out.WriteByte(Translate_ServerToTV::svc_spawnplayer);
		out.WriteByte(tr.consoleplayer);
		out.WriteShort(tr.playermobj);
		out.WriteLong(tr.spawnang);
		out.WriteLong(tr.spawnpos[0]);
		out.WriteLong(tr.spawnpos[1]);
		out.WriteLong(tr.spawnpos[2]);

		out.WriteByte(Translate_ServerToTV::svc_spectate);
		out.WriteByte(tr.consoleplayer);
		out.WriteByte(true);

		NET_SendPacket(out.cursize, out.data, net_from);
		std::cout << "shadow client connect from " <<(int)tr.consoleplayer<< NET_AdrToString(net_from) << std::endl;
	}
}

void OnPacketTV()
{
	if(NET_CompareAdr(net_from, net_server))
	{
		buf_t out = net_message;
		// if this is a server connect message, keep a copy for other clients
		int t = net_message.ReadLong();
		if(t == CHALLENGE)
		{
			std::cout << "first client connected" << std::endl;
			challenge_message = net_message;
		}
		else if(t == 0)
		{
			first_message = net_message;
			buf_t tmp;
			tr.Go(net_message, tmp);
		}
		else
		{
			out.clear();
			out.WriteLong(t);
			tr.Go(net_message, out);
		}
		// replicate server packet to all connected clients
		for(int i = 0; i < spectators.size(); i++)
		{
			NET_SendPacket(out.cursize, out.data, spectators[i]);
		}
	}
	else
	{
		// is this an existing client?
		for(int i = 0; i < spectators.size(); i++)
		{
			if(NET_CompareAdr(net_from, spectators[i]))
			{
				OnClientTV(i);
				return;
			}
		}

		// must be a new client
		OnNewClientTV();
	}
}

