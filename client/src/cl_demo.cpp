// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Functions for recording and playing back recordings of network games
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "cl_main.h"
#include "p_ctf.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "cl_demo.h"
#include "p_saveg.h"
#include "r_main.h"
#include "st_stuff.h"
#include "p_mobj.h"
#include "clc_message.h"
#include "svc_message.h"
#include "g_gametype.h"
#include "g_game.h"

#include "PacketHeaderType.h"

EXTERN_CVAR(sv_maxclients)
EXTERN_CVAR(sv_maxplayers)

// Want to press your luck loading previous-versioned netdemos?  Press this button!  Don't get a whammy!
constexpr bool TRY_LOADING_OLD_NETDEMOS = false;

extern std::string server_host;
extern std::string digest;
extern OResFiles wadfiles;

extern bool hasReceivedFullUpdate;

/**
 * @brief Map demo versions to the latest Odamex version that can read them.
 *
 * @param version Demo version to check.
 * @return Latest Odamex version for that demo in packed format, or 0 if
 *         the demo version is unknown to us.
 */
int LatestDemoVersion(const int version)
{
	switch (version)
	{
	case 4:
		return GAMEVER;
	case 3:
		return MAKEVER(12, 2, 1);
	case 2:
		return MAKEVER(0, 6, 0);
	case 1:
		return MAKEVER(0, 5, 3);
	default:
		return 0;
	}
}

NetDemo::~NetDemo()
{
	cleanUp();
}

void NetDemo::reset()
{
	cleanUp();

	filename = "";
	header = netdemo_header4_t{};
	captured.clear();
}

//
// cleanUp
//
//   Attempts to close any open files and generally exit gracefully.
//

void NetDemo::cleanUp()
{
	if (isRecording())
	{
		stopRecording();    // Try to write any unwritten data
	}

	// close all files
	demofp.close();

	snapshot_index.clear();
	map_index.clear();
	state = oldstate = NetDemo::st_stopped;
	netdemotic = pause_netdemotic = last_map_tic = 0;
	timingdemo = false;
}

/**
 * Error handler.
 *
 * Generic error handler for netdemo issues.
 *
 * @param message Error message.
 */
void NetDemo::error(const std::string &message)
{
	cleanUp();
	PrintFmt(PRINT_HIGH, "{}\n", message);
}

/**
 * Fatal error hendler.
 *
 * Error handler for netdemo issues that should blank out the view of
 * the game.  Generally used for issues that come up during playback.
 *
 * @param message Error message.
 */
void NetDemo::fatalError(const std::string &message)
{
	cleanUp();
	stopPlaying();

	PrintFmt(PRINT_HIGH, "{}\n", message);
}

//
// writeHeader()
//
//   Writes the header struct to the netdemo file in little-endian format
//   Assumes that demofp has been opened correctly elsewhere.  Does not close
//   the file.

bool NetDemo::writeHeader()
{
	memcpy(header.id.identifier, "ODAD", 4);
	header.id.version = NETDEMOVER;
	header.compression = 0;
	header.snapshot_spacing = NetDemo::SNAPSHOT_SPACING;

	demofp.seekp(0, std::ios::beg);
	const auto startingPosition = demofp.tellp();

	const bool result = startingPosition >= 0
	                    and M_WriteLE(demofp, header.id.identifier)
	                    and M_WriteLE(demofp, header.id.version)
	                    and M_WriteLE(demofp, header.compression)
	                    and M_WriteLE(demofp, header.snapshot_spacing)
	                    and M_WriteLE(demofp, header.starting_gametic)
	                    and M_WriteLE(demofp, header.ending_gametic)
	                    and M_WriteLE(demofp, header.reserved)
	                    and demofp.tellp() - startingPosition == HEADER_SIZE;
	return result;
}


bool NetDemo::netdemo_header_id_t::Read(std::fstream& io_stream)
{
	if (io_stream.good())
	{
		return  M_ReadLE(io_stream, identifier)
		    and M_ReadLE(io_stream, version);
	}
	return false;
}

bool NetDemo::netdemo_header3_t::Read(std::fstream& io_stream)
{
	if (io_stream.good())
	{
		return  id.Read(io_stream)
		    and M_ReadLE(io_stream, compression)
		    and M_ReadLE(io_stream, snapshot_index_size)
		    and M_ReadLE(io_stream, snapshot_index_offset)
		    and M_ReadLE(io_stream, map_index_size)
		    and M_ReadLE(io_stream, map_index_offset)
		    and M_ReadLE(io_stream, snapshot_spacing)
		    and M_ReadLE(io_stream, starting_gametic)
		    and M_ReadLE(io_stream, ending_gametic)
		    and M_ReadLE(io_stream, reserved);
	}
	return false;
}

bool NetDemo::netdemo_header4_t::Read(std::fstream& io_stream)
{
	if (io_stream.good())
	{
		return  id.Read(io_stream)
		    and M_ReadLE(io_stream, compression)
		    and M_ReadLE(io_stream, snapshot_spacing)
		    and M_ReadLE(io_stream, starting_gametic)
		    and M_ReadLE(io_stream, ending_gametic)
		    and M_ReadLE(io_stream, reserved);
	}
	return false;
}

//
// readHeader()
//
//   Reads the header struct from the netdemo file, converting it from
//   little-endian format to whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readHeader()
{
	demofp.seekg(0, std::ios::beg);
	const auto startingPosition = demofp.tellg();

	netdemo_header_id_t headerId;
	const bool headerIDOk = headerId.Read(demofp);

	if (not (headerIDOk
	         and headerId.identifier[0] == 'O'
	         and headerId.identifier[1] == 'D'
	         and headerId.identifier[2] == 'A'
	         and headerId.identifier[3] == 'D'))
	{
		return false;
	}

	header.id = headerId;

	if (header.id.version == NETDEMOVER)
	{
		demofp.seekg(startingPosition, std::ios::beg);

		return header.Read(demofp)
		        and demofp.tellg() - startingPosition == HEADER_SIZE;
	}

	if (header.id.version == 3)
	{
		demofp.seekg(startingPosition, std::ios::beg);

		netdemo_header3_t header3;

		if (header3.Read(demofp)
		        and demofp.tellg() - startingPosition == HEADER_SIZE)
		{
			// Translate from 3 to NETDEMOVER
			header.Import(header3);
			return true;
		}
	}
	return false;
}

//
// pouplateMessageIndexes()
//
//   called from startPlaying, seeks through the demo and populates 
//   map_index and snapshot_index vecs
void NetDemo::populateMessageIndexes()
{
	demofp.seekg(NetDemo::HEADER_SIZE, std::ios::beg);

	netdemo_message_t type;
	uint32_t len = 0, tic = 0, last_tic = 0;

	do
	{
		last_tic = tic;
		if (!readMessageHeader(type, len, tic))
		{
			break;
		}

		const std::streampos offset = demofp.tellg() - NetDemo::MESSAGE_HEADER_SIZE;

		if (type == NetDemo::msg_snapshot)
		{
			netdemo_index_entry_t entry = {tic, offset};
			snapshot_index.push_back(entry);
		}

		else if (type == NetDemo::msg_map_change)
		{
			netdemo_index_entry_t entry = {tic, offset};
			map_index.push_back(entry);
			snapshot_index.push_back(entry);
		}

		else if (type == NetDemo::msg_eof)
		{
			break;
		}

		demofp.seekg(len, std::ios::cur);
	} while (demofp.good());

	// fix for playing a demo that hard crashed and couldnt write ending_gametic
	if (header.ending_gametic == 0)
	{
		header.ending_gametic = last_tic;
	}
}

//
// startRecording()
//
//   Creates the netdemo file with the specified filename.  A temporary
//   header is written which will be overwritten with the proper information
//   in stopRecording().

bool NetDemo::startRecording(const std::string &filename)
{
	this->filename = filename;

	if (isInPlayback())
	{
		error("Cannot record a netdemo while not connected to a server.");
		return false;
	}

	// Already recording so just ignore the command
	if (isRecording())
		return true;

	demofp.close();

	demofp = std::fstream(filename,
	                      std::ios::out |
	                      std::ios::binary |
	                      std::ios::trunc);
	if (not demofp.good())
	{
		//error("Unable to create netdemo file " + filename + ".");
		I_Warning("Unable to create netdemo file {}", filename);
		return false;
	}

	header = netdemo_header4_t{};
	header.starting_gametic = gametic;

	// Note: The header is not finalized at this point.  Write it anyway to
	// reserve space in the output file for it and overwrite it later.
	if (!writeHeader())
	{
		error("Unable to write netdemo header.");
		return false;
	}

	state = NetDemo::st_recording;
	PrintFmt(PRINT_HIGH, "Recording netdemo {}.\n", filename);

	if (connected)
	{
		// write a simulation of the connection sequence since the server
		// has already sent it to the client and it wasn't captured
		static buf_t tempbuf(NETDEMO_STARTUP_PACKET_SIZE);

		// Fake the launcher query response
		SZ_Clear(&tempbuf);
		writeLauncherSequence(&tempbuf);
		capture(&tempbuf);
		writeMessages();

		// Fake the server's side of the connection sequence
		SZ_Clear(&tempbuf);
		writeConnectionSequence(&tempbuf);
		capture(&tempbuf);
		writeMessages();

		SZ_Clear(&tempbuf);
		MSG_WriteSVCBuffer(&tempbuf, odaproto::clc::NetDemoLoadSnap());
		capture(&tempbuf);
		writeMessages();

		// Record any additional messages (usually a full update if auto-recording))
		// Do not write this message immediately because it needs to be written after
		// the map snapshot.
		capture(&net_message);
	}

	return true;
}


//
// startPlaying()
//
//

bool NetDemo::startPlaying(const std::string &filename)
{
	this->filename = filename;

	if (filename.empty())
	{
		error("No netdemo filename specified.");
		return false;
	}

	if (isPlaying())
	{
		// restart playing
		cleanUp();
		return startPlaying(filename);
	}

	if (isRecording())
	{
		error("Cannot play a netdemo while recording.");
		return false;
	}

	demofp = std::fstream(filename,
	                      std::ios::in |
	                      std::ios::binary);
	if (not demofp.good())
	{
		error("Unable to open netdemo file.");
		return false;
	}

	if (!readHeader())
	{
		error("Unable to read netdemo header.");
		return false;
	}

	if constexpr (TRY_LOADING_OLD_NETDEMOS)
	{
		PrintFmt(PRINT_WARNING, "Attempting to load a version {} netdemo...\n", header.id.version);
	}
	else if (header.id.version != NETDEMOVER)
	{
		std::string buffer;
		const int latestVersion = LatestDemoVersion(header.id.version);
		if (latestVersion)
		{
			int maj, min, patch;
			BREAKVER(latestVersion, maj, min, patch);
			buffer = fmt::sprintf(
			          "This demo is too old to play in this version of Odamex.  Please "
			          "visit https://odamex.net/ to obtain Odamex %d.%d.%d or older.",
			          maj, min, patch);
		}
		else
		{
			buffer = fmt::sprintf(
			          "This demo is too new to play in this version of Odamex.  Please "
			          "visit https://odamex.net/ to obtain a newer version of Odamex.");
		}

		error(buffer);
		return false;
	}

	populateMessageIndexes();

	// get set up to read server cmds
	demofp.seekg(NetDemo::HEADER_SIZE, std::ios::beg);
	state = NetDemo::st_playing;

	PrintFmt(PRINT_HIGH, "Playing netdemo {}.\n", filename);

	return true;
}


//
// pause()
//
//   Changes the netdemo's state to paused.  No messages will be read or written
//   while in this state.

bool NetDemo::pause()
{
	if (isPlaying())
	{
		timingdemo = false;
		oldstate = state;
		state = NetDemo::st_paused;
		return true;
	}

	return false;
}


//
// resume()
//
//   Changes the netdemo's state to its state prior to the call to pause()
//

bool NetDemo::resume()
{
	if (isPaused())
	{
		pause_netdemotic = 0;
		state = oldstate;
		return true;
	}

	return false;
}

//
// stopRecording()
//
//   Writes the netdemo index to file and rewrites the netdemo header before
//   closing the netdemo file.

bool NetDemo::stopRecording()
{
	if (!isRecording())
	{
		return false;
	}
	state = NetDemo::st_stopped;

	// write any remaining messages that have been captured
	writeMessages();

	// write the end-of-demo marker - header + size
	byte stopdata[2] = {clc_netdemostop, 0};
	writeChunk(&stopdata[0], sizeof(stopdata), NetDemo::msg_eof);

	// write the number of the last gametic in the recording
	header.ending_gametic = gametic;

	demofp.flush();

	// rewrite the header for ending_gametic
	if (!writeHeader())
	{
		error("Unable to write updated netdemo header.");
		return false;
	}

	demofp.close();

	PrintFmt(PRINT_HIGH, "Demo recording has stopped.\n");
	reset();
	return true;
}


//
// stopPlaying()
//
//   Closes the netdemo file and sets the state to stopped
//

bool NetDemo::stopPlaying()
{
	state = NetDemo::st_stopped;
	SZ_Clear(&net_message);
	CL_QuitNetGame(NQ_SILENT);

	demofp.close();

	PrintFmt(PRINT_HIGH, "Demo has ended.\n");
	reset();
	gameaction = ga_fullconsole;
	gamestate = GS_FULLCONSOLE;

	return true;
}

//
// writeLocalCmd()
//
//   Generates a message indicating the current position and angle of the
//   consoleplayer, taking the place of ticcmds.
void NetDemo::writeLocalCmd(buf_t *netbuffer) const
{
	// Record the local player's data
	player_t& player = consoleplayer();
	if (not player.mo)
		return;

	MSG_WriteSVCBuffer(netbuffer, CLC_NetdemoCap(player, localcmds[gametic % MAXSAVETICS], ::messenger));
}


void NetDemo::writeChunk(const byte *data, size_t size, netdemo_message_t type)
{
	message_header_t msgheader;

	msgheader.type      = static_cast<byte>(type);
	msgheader.length    = size;
	msgheader.gametic   = gametic;

	const auto startingPosition = demofp.tellp();
	const bool headerResult = startingPosition >= 0
	                            and M_WriteLE(demofp, msgheader.type)
	                            and M_WriteLE(demofp, msgheader.length)
	                            and M_WriteLE(demofp, msgheader.gametic)
	                            and demofp.tellp() - startingPosition == MESSAGE_HEADER_SIZE;

	if (headerResult)
	{
		const auto dataStartPosition = demofp.tellp();
		demofp.write(reinterpret_cast<const char*>(data), size);
		if (demofp.tellp() - dataStartPosition != size)
		{
			error("Unable to write netdemo message chunk\n");
		}
	}
}


//
// atSnapshotInterval()
//
//    Returns true if it is the appropriate time to write a snapshot
//
bool NetDemo::atSnapshotInterval()
{
	if (!connected || last_map_tic == 0 || gamestate != GS_LEVEL)
		return false;

	if (gametic == last_map_tic)
		return false;

	return ((gametic - last_map_tic) % header.snapshot_spacing == 0);
}


bool NetDemo::ticker()
{
	if (not isInPlayback())
		return false;

	if (isPlaying())
	{
		netdemotic++;
		if (netdemotic == pause_netdemotic)
		{
			pause_netdemotic = 0;
			pause();
			::paused = true;
		}
	}
	return true;
}

//
// writeMessages()
//
//   Writes the packets received from the server and captures local player
//   input and writes to the netdemo file.
//

void NetDemo::writeMessages()
{
	if (!isRecording())
		return;

	static buf_t netbuf_localcmd(1024);

	if (atSnapshotInterval())
	{
		writeSnapshotData(snapbuf);
		writeChunk(snapbuf.data(), snapbuf.size(), NetDemo::msg_snapshot);
	}

	if (connected)
	{
		// Write the console player's game data
		SZ_Clear(&netbuf_localcmd);
		writeLocalCmd(&netbuf_localcmd);
		captured.push_back(netbuf_localcmd);
	}

	auto output_buf = std::make_unique<byte[]>(captured.size() * MAX_UDP_PACKET);

	uint32_t output_len = 0;
	while (!captured.empty())
	{
		buf_t netbuf(captured.front());
		uint32_t len = netbuf.BytesLeftToRead();

		byte *chunk = netbuf.ReadChunk(len);
		memcpy(&output_buf[output_len], chunk, len);
		output_len += len;

		captured.pop_front();
	}

	writeChunk(output_buf.get(), output_len, NetDemo::msg_packet);
}


//
// readMessageHeader()
//
//   Reads the message length and gametic from the netdemo file into the
//   len and tic parameters.
//   Returns false upon file read error.

bool NetDemo::readMessageHeader(netdemo_message_t &type, uint32_t &len, uint32_t &tic)
{
	len = tic = 0;

	message_header_t msgheader;

	const bool headerIsGood =   M_ReadLE(demofp, msgheader.type)
	                        and M_ReadLE(demofp, msgheader.length)
	                        and M_ReadLE(demofp, msgheader.gametic);
	if (not headerIsGood)
	{
		return false;
	}

	// convert the values to native byte order
	len = msgheader.length;
	tic = msgheader.gametic;
	type = static_cast<netdemo_message_t>(msgheader.type);

	return true;
}


//
// readMessageBody()
//
//   Reads a message of length len from the netdemo file and stores the
//   message in netbuffer.
//

void NetDemo::readMessageBody(buf_t *netbuffer, uint32_t len)
{
	auto msgdata = std::make_unique<char[]>(len);

	demofp.read(msgdata.get(), len);
	if (demofp.gcount() < len)
	{
		fatalError("Can not read netdemo message.");
		return;
	}

	// ensure netbuffer has enough free space to hold this packet
	if (netbuffer->maxsize() - netbuffer->size() < len)
	{
		netbuffer->resize(len + netbuffer->size() + 1, false);
	}

	netbuffer->WriteChunk(msgdata.get(), len);

	if (!connected)
	{
		int type = netbuffer->ReadLong();
		if (type == MSG_CHALLENGE)
		{
			CL_PrepareConnect();
		}
		else if (type == 0)
		{
			CL_Connect();
		}
	}
	else
	{
		last_received = gametic;
		noservermsgs = false;
		// Since packets are captured after the header is read, we do not
		// have to read the packet header
		//
		// Please note that we don't need to call CL_SaveCmd here because
		// the parse of CLC_NetdemoCap unpacks the PlayerInputs and fills
		// out the player.cmd.
		CL_ParseCommands();

		if (gametic - last_received > 65)
		{
			noservermsgs = true;
		}
	}
}


//
// readMessages()
//
//   Read the next message from the netdemo file.  The message reprepsents one
//   tic worth of network messages and one message per tic ensures the timing
//   of playback matches the timing of the messages when they were recorded.
//
//   Snapshots and map changes are skipped as they are directly read elsewhere.

void NetDemo::readMessages(buf_t* netbuffer)
{
	if (!isPlaying())
	{
		return;
	}

	netdemo_message_t type;
	uint32_t len = 0, tic = 0;

	// get the values for type, len and tic
	if (!readMessageHeader(type, len, tic))
	{
		fatalError("Failed to read netdemo message header.");
		return;
	}

	while (type == NetDemo::msg_snapshot || type == NetDemo::msg_map_change)
	{
		// skip over snapshots and read the next message instead
		demofp.seekg(len, std::ios::cur);
		if (!readMessageHeader(type, len, tic))
		{
			fatalError("Failed to read netdemo message header.");
			return;
		}
	}

	// read from the input file and put the data into netbuffer
	gametic     = tic;
	netdemotic  = gametic - header.starting_gametic;
	readMessageBody(netbuffer, len);
}


//
// capture()
//
//   Copies data from inputbuffer just before the game parses it
//

void NetDemo::capture(const buf_t* inputbuffer)
{
	if (!isRecording())
	{
		return;
	}

	if (inputbuffer->size() > 0)
	{
		captured.emplace_back(*inputbuffer);
	}
}

void NetDemo::capture(const std::basic_string<byte>& buffer)
{
	if (isRecording())
	{
		if (buffer.size() > 0)
		{
			captured.emplace_back(buffer);
		}
	}
}


//
// writeLauncherSequence()
//
//   Emulates the sequence of messages the server sends a launcher program or
//   the client when a client first contacts a server to initiate a connection.
//   As much of this data is parsed and ignored by a connecting client, a good
//   deal of the data written to netbuffer is simply place holding data and not
//   accurate.
//

void NetDemo::writeLauncherSequence(buf_t *netbuffer)
{
	// Server sends launcher info
	MSG_WriteLong   (netbuffer, PROTO_CHALLENGE);
	MSG_WriteLong   (netbuffer, 0);     // server_token

	// get sv_hostname and write it
	MSG_WriteString (netbuffer, server_host.c_str());

	int playersingame = std::count_if(players.cbegin(), players.cend(), [](const auto& player){ return player.ingame(); });
	MSG_WriteByte   (netbuffer, playersingame);
	MSG_WriteByte   (netbuffer, 0);             // sv_maxclients
	MSG_WriteString (netbuffer, level.mapname.c_str());

	// names of all the wadfiles on the server
	size_t numwads = wadfiles.size();
	if (numwads > 0xff)
	    numwads = 0xff;
	MSG_WriteByte   (netbuffer, numwads - 1);

	for (size_t n = 1; n < numwads; n++)
	{
		// Don't use absolute paths, as they present a security risk.
		MSG_WriteString(netbuffer, ::wadfiles[n].getBasename().c_str());
	}

	MSG_WriteBool   (netbuffer, 0);     // deathmatch?
	MSG_WriteByte   (netbuffer, 0);     // sv_skill
	MSG_WriteBool   (netbuffer, (sv_gametype == GM_TEAMDM));
	MSG_WriteBool   (netbuffer, (sv_gametype == GM_CTF));

	for (const auto& player : players)
	{
		// Notes: client just ignores this data but still expects to parse it
		if (player.ingame())
		{
			MSG_WriteString(netbuffer, ""); // player's netname
			MSG_WriteShort(netbuffer, 0); // player's fragcount
			MSG_WriteLong(netbuffer, 0); // player's ping
			MSG_WriteByte(netbuffer, 0); // player's team
		}
	}

	// MD5 hash sums for all the wadfiles on the server
	for (size_t n = 1; n < numwads; n++)
		MSG_WriteString(netbuffer, ::wadfiles[n].getMD5().getHexCStr());

	MSG_WriteString (netbuffer, "");    // sv_website.cstring()

	if (G_IsTeamGame())
	{
		MSG_WriteLong   (netbuffer, 0);     // sv_scorelimit
		for (size_t n = 0; n < NUMTEAMS; n++)
		{
			MSG_WriteBool   (netbuffer, false);
		}
	}

	MSG_WriteShort  (netbuffer, VERSION);

	// Note: these are ignored by clients when the client connects anyway
	// so they don't need real data
	MSG_WriteString (netbuffer, "");    // sv_email.cstring()

	MSG_WriteShort  (netbuffer, 0);     // sv_timelimit
	MSG_WriteShort  (netbuffer, 0);     // timeleft before end of level
	MSG_WriteShort  (netbuffer, 0);     // sv_fraglimit

	MSG_WriteBool   (netbuffer, false); // sv_itemrespawn
	MSG_WriteBool   (netbuffer, false); // sv_weaponstay
	MSG_WriteBool   (netbuffer, false); // sv_friendlyfire
	MSG_WriteBool   (netbuffer, false); // sv_allowexit
	MSG_WriteBool   (netbuffer, false); // sv_infiniteammo
	MSG_WriteBool   (netbuffer, false); // sv_nomonsters
	MSG_WriteBool   (netbuffer, false); // sv_monstersrespawn
	MSG_WriteBool   (netbuffer, false); // sv_fastmonsters
	MSG_WriteBool   (netbuffer, false); // sv_allowjump
	MSG_WriteBool   (netbuffer, false); // sv_freelook
	MSG_WriteBool   (netbuffer, false); // sv_waddownload -- removed
	MSG_WriteBool   (netbuffer, false); // sv_emptyreset
	MSG_WriteBool   (netbuffer, false); // sv_cleanmaps -- removed
	MSG_WriteBool   (netbuffer, false); // sv_fragexitswitch

	for (const auto& player : players)
	{
		if (player.ingame())
		{
			MSG_WriteShort(netbuffer, player.killcount);
			MSG_WriteShort(netbuffer, player.deathcount);

			int timeingame = (time(NULL) - player.JoinTime) / 60;
			if (timeingame < 0)
				timeingame = 0;
			MSG_WriteShort(netbuffer, timeingame);
		}
	}

	MSG_WriteLong(netbuffer, static_cast<uint32_t>(0x01020304));
	MSG_WriteShort(netbuffer, sv_maxplayers);

	for (const auto& player : players)
	{
		if (player.ingame())
			MSG_WriteBool(netbuffer, player.spectator);
	}

	MSG_WriteLong   (netbuffer, static_cast<uint32_t>(0x01020305));
	MSG_WriteShort  (netbuffer, 0); // join_passowrd

	MSG_WriteLong   (netbuffer, GAMEVER);

	// TODO: handle patch files
	MSG_WriteByte   (netbuffer, 0);  // patchfiles.size()
//  MSG_WriteByte   (netbuffer, patchfiles.size());

//  for (size_t n = 0; n < patchfiles.size(); n++)
//      MSG_WriteString(netbuffer, patchfiles[n].c_str());
}

//
// writeConnectionSequence()
//
//   Emulates the sequence of messages that the server sends to a client in
//   the packet with sequence number 0 and writes them to netbuffer.
//

extern int last_svgametic;

void NetDemo::writeConnectionSequence(buf_t *netbuffer)
{
	PacketHeaderType header {0};

	header.Pack(*netbuffer);

	// Server sends our player id and digest
	MSG_WriteSVCBuffer(netbuffer, SVC_ConsolePlayer(consoleplayer(), digest));

	// our userinfo
	MSG_WriteSVCBuffer(netbuffer, SVC_UserInfo(consoleplayer(), consoleplayer().GameTime));

	// Server sends its settings
	cvar_t *var = GetFirstCvar();
	while (var)
	{
		if (var->flags() & CVAR_SERVERINFO)
		{
			MSG_WriteSVCBuffer(netbuffer, SVC_ServerSettings(*var));
		}
		var = var->GetNext();
	}

	// Server tells everyone if we're a spectator
	MSG_WriteSVCBuffer(netbuffer, SVC_PlayerMembers(consoleplayer(), SVC_PM_SPECTATOR));

	// Server sends wads & map name
	MSG_WriteSVCBuffer(netbuffer, SVC_LoadMap(wadfiles, patchfiles, level.mapname.c_str(), level.time));

	// Server spawns the player
	MSG_WriteSVCBuffer(netbuffer, SVC_SpawnPlayer(consoleplayer(), last_svgametic));
}


NetDemo::SnapshotVector::const_iterator NetDemo::lookupSnapshot(const SnapshotVector& i_vector, uint32_t gameticnum) const
{
	if (gameticnum < header.starting_gametic or
	    gameticnum > header.ending_gametic or
	    i_vector.empty())
	{
		return i_vector.end();
	}

	auto iter = std::upper_bound(i_vector.begin(),
	                             i_vector.end(),
	                             gameticnum);

	// We know that the tic number is within the valid range and that there's at least one snapshot,
	// but upper_bound will return end() if the tic number is between the start of the last snapshot
	// and the ending_gametic.
	//
	// In any case, we want to return the element BEFORE the result of upper_bound, unless it's the
	// very first element.

	if (iter == i_vector.begin())
	{
		return iter;
	}
	return iter-1;
}


// getSnapshotForNetdemotic()
//
//      Returns the snapshot that preceeds the netdemoticnum parameter or returns
//      snapshot_index.end() if the netdemoticnum is out of bounds.
//
NetDemo::SnapshotVector::const_iterator NetDemo::getSnapshotForNetdemotic(uint32_t i_netdemoticnum) const
{
	return lookupSnapshot(snapshot_index, header.starting_gametic + i_netdemoticnum);
}

// getSnapshotForGametic()
//
//      Returns the snapshot that preceeds the gameticnum parameter or returns
//      snapshot_index.end() if the gameticnum is out of bounds.
//
NetDemo::SnapshotVector::const_iterator NetDemo::getSnapshotForGametic(uint32_t gameticnum) const
{
	return lookupSnapshot(snapshot_index, gameticnum);
}

//
// getCurrentSnapshotIter()
//
//      Returns the iterator into the snapshot_index vector that immediately
//      preceeds the current gametic.
//
NetDemo::SnapshotVector::const_iterator NetDemo::getCurrentSnapshotIter() const
{
	return lookupSnapshot(snapshot_index, static_cast<uint32_t>(gametic));
}

//
// getCurrentMapIter()
//
//      Returns the iterator into the map_index vector for the map that the
//      is currently being played.
//
NetDemo::SnapshotVector::const_iterator NetDemo::getCurrentMapIter() const
{
	return lookupSnapshot(map_index, static_cast<uint32_t>(gametic));
}

//
// nextTic()
//
//      Advance to the next gametic.
//
void NetDemo::nextTic()
{
	if (!isPaused())
		return;

	pause_netdemotic = netdemotic + 1;
	state = oldstate;
	::paused = false;
}

//
// prevTic()
//
//		Rewind to the previous gametic.
//		It has to rewind to the last snapshot
//		and replay from there.
//
void NetDemo::prevTic()
{
	if (!isPaused())
		return;

	seekNetdemotic(netdemotic - 1);
}

//
// nextSnapshot()
//
//      Reads the snapshot that follows the current gametic and
//      restores the world state to the snapshot
//
void NetDemo::nextSnapshot()
{
	auto currentIter = getCurrentSnapshotIter();

	if (currentIter   == snapshot_index.end() or
	    currentIter+1 == snapshot_index.end())
	{
		return;
	}

	readSnapshot(currentIter+1);
}


//
// prevSnapshot()
//
//      Reads the snapshot that preceeds the current gametic and
//      restores the world state to the snapshot
//
void NetDemo::prevSnapshot()
{
	auto iter = getCurrentSnapshotIter();

	if (iter == snapshot_index.end())        // Unlikely, but validate it anyway.
		return;

	if (iter != snapshot_index.begin())
	{
		iter -= 1;
	}

	readSnapshot(iter);
}

//
// nextMap()
//
//      Reads the snapshot at the begining of the next map and
//      restores the world state to the snapshot
//
void NetDemo::nextMap()
{
	auto iter = getCurrentMapIter();
	if (iter == map_index.end())
		return;

	iter += 1;

	if (iter == map_index.end())
		return;

	readSnapshot(iter);
}

//
// prevMap()
//
//      Reads the snapshot at the begining of the previous map and
//      restores the world state to the snapshot
//
void NetDemo::prevMap()
{
	auto iter = getCurrentMapIter();

	if (iter == map_index.end())
		return;

	if (iter != map_index.begin())
	{
		iter -= 1;
	}

	readSnapshot(iter);
}


//
// readSnapshot()
//
//
bool NetDemo::readSnapshot(SnapshotVector::const_iterator snap)
{
	if (not isPlaying())
		return false;

	gametic = snap->ticnum;
	int file_offset = snap->offset;
	demofp.seekg(file_offset, std::ios::beg);

	// read the values for length, gametic, and message type
	netdemo_message_t type;
	uint32_t len = 0, tic = 0;
	if (!readMessageHeader(type, len, tic))
	{
		fatalError("Failed to read netdemo message header.");
		return false;
	}

	// Clear the snapshot buffer and read into it.
	snapbuf.clear();
	snapbuf.resize(len);

	demofp.read(reinterpret_cast<char*>(snapbuf.data()), len);
	if (demofp.gcount() < len)
	{
		fatalError("Unable to read snapshot from data file");
		return false;
	}

	readSnapshotData(snapbuf);
	netdemotic = snap->ticnum - header.starting_gametic;
	return true;
}


//
// calculateTotalTime()
//
//   Returns the total length of the demo in seconds
//
int NetDemo::calculateTotalTime() const
{
	if (not isInPlayback())
		return 0;

	return ((header.ending_gametic - header.starting_gametic) / TICRATE);
}


//
// calculateTimeElapsed()
//
//   Returns the number of seconds since the demo started playing
//
int NetDemo::calculateTimeElapsed() const
{
	if (not isInPlayback())
		return 0;

	int elapsed = netdemotic / TICRATE;
	int totaltime = calculateTotalTime();

	if (elapsed > totaltime)
		return totaltime;

	return elapsed;
}

const std::vector<int> NetDemo::getMapChangeTimes() const
{
	std::vector<int> times;

	for (const auto [ticnum, _] : map_index)
	{
		int start_time = (ticnum - header.starting_gametic) / TICRATE;
		times.push_back(start_time);
	}

	return times;
}

bool NetDemo::seekGametic(int requestedGametic)
{
	if (not isInPlayback()
	    or requestedGametic < header.starting_gametic
	    or requestedGametic > header.ending_gametic)
	{
		return false;
	}

	if (requestedGametic == gametic)
		return true;

	auto snapshotIter = getSnapshotForGametic(requestedGametic);
	if (snapshotIter == snapshot_index.end())
		return false;

	// First, we have to be playing to load a snapshot.  Then we have to be playing to
	// fast-forward to the requested tic.  If we fail, we just simply pause.
	resume();

	auto currentSnapshotIter = getCurrentSnapshotIter();

	// We want to force a snapshot load if we need to skip backwards by any amount or if
	// we're advancing to another snapshot, and the target is more than a second out.
	// The only reason for the one second out is that we can just easily fast-forward
	// 35 tics.  It's pretty arbitary really.
	const bool mustLoadSnapshot = requestedGametic < gametic
	                              or (snapshotIter != currentSnapshotIter
	                                  and requestedGametic > gametic + TICRATE);

	const bool isReadyToFF = mustLoadSnapshot ? readSnapshot(snapshotIter) : true;

	if (isReadyToFF)
	{
		// FIXME:   If we try to pause at the very beginning of a snapshot, we get a horrible
		//          view interpolation error.  Workaround: advance one tic.
		if (requestedGametic == snapshotIter->ticnum)
		{
			requestedGametic += 1;
		}
		timingdemo = true;
		pause_netdemotic = requestedGametic - header.starting_gametic;

		return true;
	}
	pause();
	return false;
}

bool NetDemo::seekNetdemotic(int requestedNetdemotic)
{
	return seekGametic(requestedNetdemotic + header.starting_gametic);
}

void NetDemo::writeMapChange()
{
	if (connected && gamestate == GS_LEVEL)
	{
		writeSnapshotData(snapbuf);
		writeChunk(snapbuf.data(), snapbuf.size(), NetDemo::msg_map_change);
		last_map_tic = gametic;
	}
}

void NetDemo::writeIntermission()
{
	if (connected && gamestate == GS_INTERMISSION)
	{
		writeSnapshotData(snapbuf);
		writeChunk(snapbuf.data(), snapbuf.size(), NetDemo::msg_snapshot);
	}
}

//
// writeSnapshotData()
//
//   Write the entire state of the game to netbuffer.  Called by
//   writeSnapshot() and used to simulate SV_ClientFullUpdate() when
//   writing the connection sequence at the start of a netdemo.
//

void NetDemo::writeSnapshotData(std::vector<byte>& buf)
{
	G_SnapshotLevel();

	FLZOMemFile memfile;
	memfile.Open();         // open for writing

	FArchive arc(memfile);

	// Save the server cvars
	byte vars[4096], *vars_p;
	vars_p = vars;

	cvar_t::C_WriteCVars(&vars_p, CVAR_SERVERINFO, 4096);
	arc.WriteCount(vars_p - vars);
	arc.Write(vars, vars_p - vars);

	// write wad info
	arc << static_cast<byte>(wadfiles.size() - 1);
	for (size_t i = 1; i < wadfiles.size(); i++)
	{
		arc << D_CleanseFileName(::wadfiles[i].getBasename()).c_str();
		arc << ::wadfiles[i].getMD5().getHexCStr();
	}

	arc << static_cast<byte>(patchfiles.size());
	for (const auto& file : patchfiles)
	{
		arc << D_CleanseFileName(file.getBasename()).c_str();
		arc << file.getMD5().getHexCStr();
	}

	// write map info
	arc << level.mapname.c_str();
	arc << static_cast<byte>(gamestate == GS_INTERMISSION);

	G_SerializeSnapshots(arc);
	P_SerializeRNGState(arc);
	P_SerializeACSDefereds(arc);
	P_SerializeHorde(arc);

	// Save the status of the flags in CTF
	for (int i = 0; i < NUMTEAMS; i++)
		arc << GetTeamInfo(static_cast<team_t>(i))->FlagData;

	// Save team points
	for (int i = 0; i < NUMTEAMS; i++)
		arc << GetTeamInfo(static_cast<team_t>(i))->Points;

	arc << level.time;

	for (int i = 0; i < NUM_WORLDVARS; i++)
	{
		arc << ACS_WorldVars[i];
		ACSWorldGlobalArray worldarr = ACS_WorldArrays[i];
		arc << worldarr.size();
		for (const auto& [key, val] : worldarr)
		{
			arc << key;
			arc << val;
		}
	}


	for (int i = 0; i < NUM_GLOBALVARS; i++)
	{
		arc << ACS_GlobalVars[i];
		ACSWorldGlobalArray globalarr = ACS_GlobalArrays[i];
		arc << globalarr.size();
		for (const auto& [key, val] : globalarr)
		{
			arc << key;
			arc << val;
		}
	}

	arc << rollerState;

	byte check = 0x1d;
	arc << check;          // consistancy marker

	arc.Close();

	// Resize the snapshot buffer to hold our snapshot size.
	buf.resize(memfile.Length());
	memfile.WriteToBuffer(buf.data(), buf.size());

	if (level.info->snapshot != NULL)
	{
		delete level.info->snapshot;
		level.info->snapshot = NULL;
	}
}


void NetDemo::readSnapshotData(std::vector<byte>& buf)
{
	byte cid = consoleplayer_id;
	byte did = displayplayer_id;

	P_ClearAllNetIds();

	// Remove all players
	players.clear();

	CL_ResetWorldPrediction();

	// Remove all actors
	TThinkerIterator<AActor> iterator;
	AActor *mo;
	while ( (mo = iterator.Next() ) )
		mo->Destroy();

	gameaction = ga_nothing;

	FLZOMemFile memfile;

	memfile.Open(buf.data()); // open for reading

	FArchive arc(memfile);

	// Read the server cvars
	byte vars[4096], *vars_p;
	vars_p = vars;
	size_t len = arc.ReadCount ();
	arc.Read(vars, len);
	cvar_t::C_ReadCVars(&vars_p);

	// read wad info
	OWantFiles newwadfiles, newpatchfiles;
	byte numwads, numpatches;
	std::string res, hashStr;

	arc >> numwads;
	for (size_t i = 0; i < numwads; i++)
	{
		arc >> res;
		arc >> hashStr;

		OMD5Hash hash;
		OMD5Hash::makeFromHexStr(hash, hashStr);

		OWantFile want;
		OWantFile::makeWithHash(want, res, OFILE_WAD, hash);
		newwadfiles.push_back(want);
	}

	arc >> numpatches;
	for (size_t i = 0; i < numpatches; i++)
	{
		arc >> res;
		arc >> hashStr;

		OMD5Hash hash;
		OMD5Hash::makeFromHexStr(hash, hashStr);

		OWantFile want;
		OWantFile::makeWithHash(want, res, OFILE_DEH, hash);
		newpatchfiles.push_back(want);
	}

	std::string mapname;
	bool intermission;
	arc >> mapname;
	arc >> intermission;

	G_SerializeSnapshots(arc);
	P_SerializeRNGState(arc);
	P_SerializeACSDefereds(arc);
	P_SerializeHorde(arc);

	// Read the status of flags in CTF
	for (int i = 0; i < NUMTEAMS; i++)
		arc >> GetTeamInfo(static_cast<team_t>(i))->FlagData;

	// Read team points
	for (int i = 0; i < NUMTEAMS; i++)
		arc >> GetTeamInfo(static_cast<team_t>(i))->Points;

	arc >> level.time;

	for (int i = 0; i < NUM_WORLDVARS; i++)
	{
		arc >> ACS_WorldVars[i];
		int size, k, v;
		arc >> size;
		for (int j = 0; j < size; j++)
		{
			arc >> k;
			arc >> v;
			ACS_WorldArrays[i][k] = v;
		}
	}

	for (int i = 0; i < NUM_GLOBALVARS; i++)
	{
		arc >> ACS_GlobalVars[i];
		int size, k, v;
		arc >> size;
		for (int j = 0; j < size; j++)
		{
			arc >> k;
			arc >> v;
			ACS_GlobalArrays[i][k] = v;
		}
	}

	arc >> rollerState;

	multiplayer = true;

	// load a base level
	savegamerestore = true;     // Use the player actors in the savegame
	serverside = false;

	G_LoadWad(newwadfiles, newpatchfiles);

	G_InitNew(mapname);
	displayplayer_id = consoleplayer_id = 1;
	savegamerestore = false;

	// read consistancy marker
	byte check;
	arc >> check;

	arc.Close();

	if (check != 0x1d)
		fatalError("Bad snapshot");

	consoleplayer_id = cid;

	// try to restore display player
	player_t *disp = &idplayer(did);
	if (validplayer(*disp) && disp->ingame() && !disp->spectator)
		displayplayer_id = did;
	else
		displayplayer_id = cid;

	// setup psprites and restore player colors
	for (auto& player : players)
	{
		P_SetupPsprites(player);
		R_BuildPlayerTranslation(player.id, CL_GetPlayerColor(player), player.userinfo.colorpreset);
	}

	R_CopyTranslationRGB(menuplayer_id, consoleplayer_id);

	// Link the CTF flag actors to CTFdata[i].actor
	TThinkerIterator<AActor> flagiterator;
	while ( (mo = flagiterator.Next() ) )
	{
		for (int iTeam = 0; iTeam < NUMTEAMS; iTeam++)
		{
			TeamInfo* teamInfo = GetTeamInfo(static_cast<team_t>(iTeam));
			if (mo->sprite == teamInfo->FlagDownSprite || mo->sprite == teamInfo->FlagCarrySprite)
				teamInfo->FlagData.actor = mo->ptr();
		}
	}

	// Make sure the status bar is displayed correctly
	R_ForceViewWindowResize();
	ST_Start();

	// Make sure the message handling understands that the player is fully up-to-date.
	// Especially important for rollback replication.
	::hasReceivedFullUpdate = true;
}

VERSION_CONTROL (cl_demo_cpp, "$Id$")
