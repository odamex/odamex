// Emacs style mode select   -*- C++ -*-
// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2010 by The Odamex Team.
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

#include "doomtype.h"
#include "cl_main.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "c_dispatch.h"
#include "d_net.h"
#include "cl_demo.h"
#include "m_swap.h"
#include "version.h"

NetDemo::NetDemo() : state(stopped), filename(""), demofp(NULL)
{
}

NetDemo::~NetDemo()
{
	cleanUp();
}


//
// copy
//
//   Copies the data from one NetDemo object to another
 
void NetDemo::copy(NetDemo &to, const NetDemo &from)
{
	// free any memory used by structures and close open files
	cleanUp();

	to.state 			= from.state;
	to.oldstate			= from.oldstate;
	to.filename			= from.filename;
	to.demofp			= from.demofp;
	to.cmdbuf			= from.cmdbuf;
	to.bufcursor		= from.bufcursor;
	to.captured			= from.captured;
	to.snapshot_index	= from.snapshot_index;
	memcpy(&to.header, &from.header, NetDemo::HEADER_SIZE);
}


NetDemo::NetDemo(const NetDemo &rhs)
{
	copy(*this, rhs);
}

NetDemo& NetDemo::operator=(const NetDemo &rhs)
{
	copy(*this, rhs);
	return *this;
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
		stopRecording();	// Try to write any unwritten data
	}
	
	// close all files
	if (demofp)
	{
		fclose(demofp);
		demofp = NULL;
	}
	
	snapshot_index.clear();
	state = NetDemo::stopped;
}



void NetDemo::error(const std::string &message)
{
	cleanUp();
	gameaction = ga_nothing;
	gamestate = GS_FULLCONSOLE;

	Printf(PRINT_HIGH, "%s\n", message.c_str());
}


//
// writeHeader()
//
//   Writes the header struct to the netdemo file in little-endian format
//   Assumes that demofp has been opened correctly elsewhere.  Does not close
//   the file.

bool NetDemo::writeHeader()
{
	strncpy(header.identifier, "ODAD", 4);
	header.version = NETDEMOVER;
	header.compression = 0;
	header.snapshot_spacing = NetDemo::SNAPSHOT_SPACING;

	netdemo_header_t tmpheader;
	memcpy(&tmpheader, &header, NetDemo::HEADER_SIZE);
	// convert from native byte ordering to little-endian
	tmpheader.snapshot_index_offset = LONG(tmpheader.snapshot_index_offset);
	tmpheader.snapshot_index_size = LONG(tmpheader.snapshot_index_size);
	tmpheader.snapshot_spacing = SHORT(tmpheader.snapshot_spacing);
	tmpheader.first_gametic = LONG(tmpheader.first_gametic);
	
	fseek(demofp, 0, SEEK_SET);
	size_t cnt = fwrite(&tmpheader, 1, NetDemo::HEADER_SIZE, demofp);

	if (cnt < NetDemo::HEADER_SIZE)
	{
		return false;
	}

	return true;
}


//
// readHeader()
//
//   Reads the header struct from the netdemo file, converting it from
//   little-endian format to whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readHeader()
{
	fseek(demofp, 0, SEEK_SET);

	size_t cnt = fread(&header, 1, NetDemo::HEADER_SIZE, demofp);
	if (cnt < NetDemo::HEADER_SIZE)
	{
		return false;
	}

	// convert from little-endian to native byte ordering
	header.snapshot_index_offset = LONG(header.snapshot_index_offset);
	header.snapshot_index_size = LONG(header.snapshot_index_size);
	header.snapshot_spacing = SHORT(header.snapshot_spacing);
	header.first_gametic = LONG(header.first_gametic);

	return true;
}


//
// writeIndex()
//
//   Writes the snapshot index to the netdemo file, converting it to
//   little-endian format from whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::writeIndex()
{
	fseek(demofp, header.snapshot_index_offset, SEEK_SET);

	for (size_t i = 0; i < snapshot_index.size(); i++)
	{
		netdemo_snapshot_entry_t entry;
		// convert to little-endian
		entry.ticnum = LONG(snapshot_index[i].ticnum);
		entry.offset = LONG(snapshot_index[i].offset);
		
		size_t cnt = fwrite(&entry, NetDemo::INDEX_ENTRY_SIZE, 1, demofp);
		if (cnt < 1)
		{
			return false;
		}
	}

	return true;
}


//
// readIndex()
//
//   Reads the snapshot index from the netdemo file, converting it from
//   little-endian format to whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readIndex()
{
	fseek(demofp, header.snapshot_index_offset, SEEK_SET);

	int num_snapshots = header.snapshot_index_size / NetDemo::INDEX_ENTRY_SIZE;
	for (int i = 0; i < num_snapshots; i++)
	{
		netdemo_snapshot_entry_t entry;
		size_t cnt = fread(&entry, NetDemo::INDEX_ENTRY_SIZE, 1, demofp);
		if (cnt < 1)
		{
			return false;
		}

		// convert from little-endian to native
		entry.ticnum = LONG(entry.ticnum);	
		entry.offset = LONG(entry.offset);

		snapshot_index.push_back(entry);
	}

	return true;
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

	if (isPlaying() || isPaused())
	{
		error("Cannot record a netdemo while not connected to a server.");
		return false;
	}

	// Already recording so just ignore the command
	if (isRecording())
		return true;

	if (demofp != NULL)		// file is already open for some reason
	{
		fclose(demofp);
		demofp = NULL;
	}

	demofp = fopen(filename.c_str(), "wb");
	if (!demofp)
	{
		error("Unable to create netdemo file.");
		return false;
	}

	memset(&header, 0, NetDemo::HEADER_SIZE);
	// Note: The header is not finalized at this point.  Write it anyway to
	// reserve space in the output file for it and overwrite it later.
	if (!writeHeader())
	{
		error("Unable to write netdemo header.");
		return false;
	}

	state = NetDemo::recording;
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

	if (!(demofp = fopen(filename.c_str(), "rb")))
	{
		error("Unable to open netdemo file.");
		return false;
	}

	if (!readHeader())
	{
		error("Unable to read netdemo header.");
		return false;
	}

    if (header.version != NETDEMOVER)
    {
        // Do nothing since there is only one version of netdemo files currently
    }

	// read the demo's index
	if (fseek(demofp, header.snapshot_index_offset, SEEK_SET) != 0)
	{
		error("Unable to find netdemo index.\n");
		return false;
	}

	if (!readIndex())
	{
		error("Unable to read netdemo index.\n");
		return false;
	}

	// get set up to read server cmds
	fseek(demofp, NetDemo::HEADER_SIZE, SEEK_SET);
	state = NetDemo::playing;
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
		oldstate = state;
		state = NetDemo::paused;
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
	state = NetDemo::stopped;

	// write any remaining messages that have been captured
	writeMessages();

	byte marker = svc_netdemostop;
	uint32_t len = sizeof(marker);
	len = LONG(len);
	uint32_t tic = LONG(gametic);

	fwrite(&len, sizeof(len), 1, demofp);
	fwrite(&tic, sizeof(tic), 1, demofp);
	// write the end-of-demo marker
	fwrite(&marker, sizeof(marker), 1, demofp);

	// tack the snapshot index onto the end of the recording
	header.snapshot_index_offset = ftell(demofp);
	header.snapshot_index_size = snapshot_index.size() * NetDemo::INDEX_ENTRY_SIZE;

	if (!writeIndex())
	{
		error("Unable to write netdemo index.");
		return false;
	}

	// rewrite the header since snapsnot_index_offset and 
	// snapshot_index_size are now known
	if (!writeHeader())
	{
		error("Unable to write updated netdemo header.");
		return false;
	}

	fclose(demofp);
	demofp = NULL;

	Printf(PRINT_HIGH, "Demo recording has stopped.\n");
	return true;
}


//
// stopPlaying()
//
//   Closes the netdemo file and sets the state to stopped
//

bool NetDemo::stopPlaying()
{
	state = NetDemo::stopped;
	SZ_Clear(&net_message);
	CL_QuitNetGame();

	if (demofp)
	{
		fclose(demofp);
		demofp = NULL;
	}
	
	Printf(PRINT_HIGH, "Demo has ended.\n");
    gameaction = ga_fullconsole;
    gamestate = GS_FULLCONSOLE;
	return true;
}


//
// writeSnapshot()
//
//   Write the entire state of the game to the netdemo file
//

void NetDemo::writeSnapshot(buf_t *netbuffer)
{
/*	// DEBUG
	Printf(PRINT_HIGH, "Writing snapshot at tic %d\n", gametic);

	// Update the netdemo's snapshot index
	netdemo_snapshot_entry_t entry;
	entry.offset = ftell(demofp);
	entry.ticnum = gametic;
	snapshot_index.push_back(entry);
	
	buf_t tempbuf(MAX_SNAPSHOT_SIZE);
	P_WriteClientFullUpdate(tempbuf, consoleplayer());

	uint32_t snapshot_datalen = tempbuf.size();

	MSG_WriteByte(netbuffer, svc_netdemosnapshot);
	MSG_WriteLong(netbuffer, snapshot_datalen);

	MSG_WriteChunk(netbuffer, tempbuf.data, snapshot_datalen);

	//[SL] DEBUG
	Printf(PRINT_HIGH, "Writing %d bytes for snapshot\n", snapshot_datalen); */
}

void NetDemo::writeLocalCmd(buf_t *netbuffer) const
{
	// Record the local player's data
	player_t *player = &consoleplayer();
	if (!player->mo)
		return;

	AActor *mo = player->mo;

	MSG_WriteByte(netbuffer, svc_netdemocap);
	MSG_WriteByte(netbuffer, player->cmd.ucmd.buttons);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.yaw);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.forwardmove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.sidemove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.upmove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.roll);

	MSG_WriteByte(netbuffer, mo->waterlevel);
	MSG_WriteLong(netbuffer, mo->x);
	MSG_WriteLong(netbuffer, mo->y);
	MSG_WriteLong(netbuffer, mo->z);
	MSG_WriteLong(netbuffer, mo->momx);
	MSG_WriteLong(netbuffer, mo->momy);
	MSG_WriteLong(netbuffer, mo->momz);
	MSG_WriteLong(netbuffer, mo->angle);
	MSG_WriteLong(netbuffer, mo->pitch);
	MSG_WriteLong(netbuffer, mo->roll);
	MSG_WriteLong(netbuffer, player->viewheight);
	MSG_WriteLong(netbuffer, player->deltaviewheight);
	MSG_WriteLong(netbuffer, player->jumpTics);
	MSG_WriteLong(netbuffer, mo->reactiontime);
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
	{
		return;
	}

	if (connected && gamestate == GS_LEVEL)
	{
		// Write the player's ticcmd
		buf_t netbuf_localcmd(128);
		writeLocalCmd(&netbuf_localcmd);
		captured.push_back(netbuf_localcmd);

		if ((gametic - header.first_gametic) % header.snapshot_spacing == 0
			&& (unsigned)gametic > header.first_gametic)
		{
			buf_t netbuf_snapshot(NetDemo::MAX_SNAPSHOT_SIZE);
			writeSnapshot(&netbuf_snapshot);
			captured.push_front(netbuf_snapshot);
		}
	}


	byte *output_buf = new byte[captured.size() * MAX_UDP_PACKET];

	uint32_t output_len = 0;
	while (!captured.empty())
	{
		buf_t netbuf(captured.front());
		uint32_t len = netbuf.BytesLeftToRead();

		byte *chunk = netbuf.ReadChunk(len);
		memcpy(output_buf + output_len, chunk, len);
		output_len += len;
		
		captured.pop_front();
	}

	uint32_t le_output_len = LONG(output_len);
	uint32_t le_gametic = LONG(gametic);

	// write a blank message even if there is no data in order to preserve
	// timing of the netdemo
	if (header.first_gametic == 0)
		header.first_gametic = gametic;

	size_t cnt = 0;
	cnt += sizeof(le_output_len) * fwrite(&le_output_len, sizeof(le_output_len), 1, demofp);
	cnt += sizeof(le_gametic) * fwrite(&le_gametic, sizeof(le_gametic), 1, demofp);

	cnt += fwrite(output_buf, 1, output_len, demofp);

	delete [] output_buf;
}


//
// readMessageHeader()
//
// Reads the message length and gametic from the netdemo file into the
// len and tic parameters.
// Returns false upon file read error.

bool NetDemo::readMessageHeader(uint32_t &len, uint32_t &tic) const
{
	len = tic = 0;

	size_t cnt = 0;
	cnt += sizeof(len) * fread(&len, sizeof(len), 1, demofp);
	cnt += sizeof(tic) * fread(&tic, sizeof(tic), 1, demofp);

	if (cnt < sizeof(len) + sizeof(tic))
	{
		return false;
	}

	// convert the values to native byte order
	len = LONG(len);
	tic = LONG(tic);

	return true;
}


//
// readMessageBody()
//
// Reads a message of length len from the netdemo file and stores the
// message in netbuffer.
//
 
void NetDemo::readMessageBody(buf_t *netbuffer, uint32_t len)
{
	char *msgdata = new char[len];
	
	size_t cnt = fread(msgdata, 1, len, demofp);
	if (cnt < len)
	{
		error("Can not read netdemo message.");

        delete[] msgdata;

		return;
	}

	// ensure netbuffer has enough free space to hold this packet
	if (netbuffer->maxsize() - netbuffer->size() < len)
	{
		netbuffer->resize(len + netbuffer->size() + 1, false);
	}

	netbuffer->WriteChunk(msgdata, len);
	delete [] msgdata;

	if (!connected)
	{
		int type = MSG_ReadLong();
		if (type == CHALLENGE)
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
		CL_ParseCommands();
		CL_SaveCmd();
		if (gametic - last_received > 65)
		{
			noservermsgs = true;
		}
	}
}


//
// readMessages()
//
//

void NetDemo::readMessages(buf_t* netbuffer)
{
	if (!isPlaying())
	{
		return;
	}

	uint32_t len, tic;
	
	readMessageHeader(len, tic);
	gametic = tic;
	readMessageBody(netbuffer, len);
}


//
// capture()
//
// Copies data from inputbuffer just before the game parses it 
//

void NetDemo::capture(const buf_t* inputbuffer)
{
	if (!isRecording())
	{
		return;
	}

	if (gamestate == GS_DOWNLOAD)
	{
		// NullPoint: I think this will skip the downloading process
		return;
	}

	if (inputbuffer->size() > 0)
	{
		captured.push_back(*inputbuffer);
	}
}


//
// snapshotLookup()
//
// Retrieves the offset in demofp of the snapshot closest to ticnum
//

int NetDemo::snapshotLookup(int ticnum)
{
	int index = ((ticnum - header.first_gametic) / header.snapshot_spacing) - 1;

	if (index >= 0 && index < (int)snapshot_index.size())
		return snapshot_index[index].offset;

	// not found
	return -1;
}


void NetDemo::skipTo(buf_t *netbuffer, int ticnum)
{
	int offset = snapshotLookup(ticnum);

	if (offset < 0)
		return;

	SZ_Clear(netbuffer);
	
	fseek(demofp, offset, SEEK_SET);
	
	// let nature take its course the next time readMessages is called
}



//
// readSnapshot()
//
//
void NetDemo::readSnapshot(buf_t *netbuffer, int ticnum)
{
/*	if (!isPlaying())
		return;

	// Remove all players
	players.clear();

	// Remove all actors
	TThinkerIterator<AActor> iterator;
	AActor *mo;
	while ( (mo = iterator.Next() ) )
	{
		mo->Destroy();
	}

	// Read the snapshot from the demo file into netbuffer
	int file_offset = snapshotLookup(ticnum);
	if (!file_offset)	// could not find a snapshot for that tic
		return;

	fseek(demofp, file_offset, SEEK_SET);
	byte marker;
	fread(&marker, sizeof(byte), 1, demofp);	// should be svc_netdemosnapshot
	size_t snapshot_datalen;
	fread(&snapshot_datalen, sizeof(size_t), 1, demofp);
	
	// TODO: resize netbuffer if it is too small to hold the snapshot
	netbuffer->clear();
	netbuffer->cursize = snapshot_datalen;
	netbuffer->readpos = 0;
	fread(netbuffer->data, 1, snapshot_datalen, demofp); */
}


VERSION_CONTROL (cl_demo_cpp, "$Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $")
