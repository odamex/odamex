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

	to.state 		= from.state;
	to.oldstate		= from.oldstate;
	to.filename		= from.filename;
	to.demofp		= from.demofp;
	to.cmdbuf		= from.cmdbuf;
	to.bufcursor	= from.bufcursor;
	to.netbuf		= from.netbuf;
	to.index		= from.index;
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
	
	index.clear();
	state = NetDemo::stopped;
}



void NetDemo::error(const std::string message)
{
	cleanUp();
	gameaction = ga_nothing;
	gamestate = GS_FULLCONSOLE;

	Printf(PRINT_HIGH, "%s\n", message.c_str());
}


//
// writeHeader()
//
// Writes the header struct to the netdemo file in little-endian format
// Assumes that demofp has been opened correctly elsewhere.  Does not close
// the file.

bool NetDemo::writeHeader()
{
	strncpy(header.identifier, "ODAD", 4);
	header.version = NETDEMOVER;
	header.compression = 0;

	netdemo_header_t tmpheader;
	memcpy(&tmpheader, &header, NetDemo::HEADER_SIZE);
	// convert from native byte ordering to little-endian
	tmpheader.index_offset = LONG(tmpheader.index_offset);
	tmpheader.index_size = LONG(tmpheader.index_size);
	tmpheader.index_spacing = SHORT(tmpheader.index_spacing);

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
// Reads the header struct from the netdemo file, converting it from
// little-endian format to whatever the client's architecture uses.  Assumes
// that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readHeader()
{
	fseek(demofp, 0, SEEK_SET);

	size_t cnt = fread(&header, 1, NetDemo::HEADER_SIZE, demofp);
	if (cnt < NetDemo::HEADER_SIZE)
	{
		return false;
	}

	// convert from little-endian to native byte ordering
	header.index_offset = LONG(header.index_offset);
	header.index_size = LONG(header.index_size);
	header.index_spacing = SHORT(header.index_spacing);

	return true;
}


//
// writeIndex()
//
// Writes the snapshot index to the netdemo file, converting it to
// little-endian format from whatever the client's architecture uses.  Assumes
// that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::writeIndex()
{
	fseek(demofp, header.index_offset, SEEK_SET);

	for (size_t i = 0; i < index.size(); i++)
	{
		netdemo_index_entry_t entry;
		// convert to little-endian
		entry.ticnum = LONG(index[i].ticnum);
		entry.offset = LONG(index[i].offset);
		
		size_t cnt = fwrite(&entry, NetDemo::INDEX_ENTRY_SIZE, 1, demofp);
		if (cnt < NetDemo::INDEX_ENTRY_SIZE)
		{
			return false;
		}
	}

	return true;
}


//
// readIndex()
//
// Reads the snapshot index from the netdemo file, converting it from
// little-endian format to whatever the client's architecture uses.  Assumes
// that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readIndex()
{
	fseek(demofp, header.index_offset, SEEK_SET);

	int num_snapshots = header.index_size / NetDemo::INDEX_ENTRY_SIZE;
	for (int i = 0; i < num_snapshots; i++)
	{
		netdemo_index_entry_t entry;
		size_t cnt = fread(&entry, NetDemo::INDEX_ENTRY_SIZE, 1, demofp);
		if (cnt < NetDemo::INDEX_ENTRY_SIZE)
		{
			return false;
		}

		// convert from little-endian to native
		entry.ticnum = LONG(entry.ticnum);	
		entry.offset = LONG(entry.offset);

		index.push_back(entry);
	}

	return true;
}


//
// startRecording()
//
// Creates the netdemo file with the specified filename.  A temporary
// header is written which will be overwritten with the proper information
// in stopRecording().

bool NetDemo::startRecording(const std::string filename)
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

bool NetDemo::startPlaying(const std::string filename)
{
	this->filename = filename;

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
	if (fseek(demofp, header.index_offset, SEEK_SET) != 0)
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
// Changes the netdemo's state to paused.  No messages will be read or written
// while in this state.

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
// Changes the netdemo's state to its state prior to the call to pause()
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
// Writes the netdemo index to file and rewrites the netdemo header before
// closing the netdemo file.

bool NetDemo::stopRecording()
{
	if (!isRecording())
	{
		return false;
	}
	state = NetDemo::stopped;

	byte marker = svc_netdemostop;
	uint32_t len = LONG(sizeof(marker));
	uint32_t tic = LONG(gametic);

	fwrite(&len, sizeof(len), 1, demofp);
	fwrite(&tic, sizeof(tic), 1, demofp);
	// write the end-of-demo marker
	fwrite(&marker, sizeof(marker), 1, demofp);

	// tack the snapshot index onto the end of the recording
	header.index_offset = ftell(demofp);
	header.index_size = index.size() * NetDemo::INDEX_ENTRY_SIZE;

	if (!writeIndex())
	{
		error("Unable to write netdemo index.");
		return false;
	}

	// rewrite the header since index_offset and index_size is now known
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
// Closes the netdemo file and sets the state to stopped
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
// writeFullUpdate()
//
// Write the entire state of the game to the netdemo file
//

void NetDemo::writeFullUpdate(int ticnum)
{
	// Update the netdemo's index
	netdemo_index_entry_t index_entry;
	index_entry.offset = ftell(demofp);
	index_entry.ticnum = ticnum;
	index.push_back(index_entry);

	// TODO: simulate the server's update messages based on what the client
	// knows of the world	
}


//
// writeMessages()
//
// Captures the net packets and local movements and writes them to the
// netdemo file.
// 

void NetDemo::writeMessages(buf_t *netbuffer, bool usercmd)
{
	if (!isRecording())
	{
		return;
	}

	buf_t netbuffertemp;
	size_t len = netbuffer->cursize;	
	
	const int safetymargin = 128;	// extra space to accomodate usercmd
	netbuffertemp.resize(len + safetymargin);
	memcpy(netbuffertemp.data, netbuffer->data, netbuffer->cursize);
	netbuffertemp.cursize = netbuffer->cursize;

	// Record the local player's data
	player_t *player = &consoleplayer();
    if (player->mo && usercmd)
    {
		AActor *mo = player->mo;

        MSG_WriteByte(&netbuffertemp, svc_netdemocap);
        MSG_WriteByte(&netbuffertemp, player->cmd.ucmd.buttons);
        MSG_WriteShort(&netbuffertemp, player->cmd.ucmd.yaw);
        MSG_WriteShort(&netbuffertemp, player->cmd.ucmd.forwardmove);
        MSG_WriteShort(&netbuffertemp, player->cmd.ucmd.sidemove);
        MSG_WriteShort(&netbuffertemp, player->cmd.ucmd.upmove);
        MSG_WriteShort(&netbuffertemp, player->cmd.ucmd.roll);

        MSG_WriteByte(&netbuffertemp, mo->waterlevel);
        MSG_WriteLong(&netbuffertemp, mo->x);
        MSG_WriteLong(&netbuffertemp, mo->y);
        MSG_WriteLong(&netbuffertemp, mo->z);
        MSG_WriteLong(&netbuffertemp, mo->momx);
        MSG_WriteLong(&netbuffertemp, mo->momy);
        MSG_WriteLong(&netbuffertemp, mo->momz);
        MSG_WriteLong(&netbuffertemp, mo->angle);
        MSG_WriteLong(&netbuffertemp, mo->pitch);
        MSG_WriteLong(&netbuffertemp, mo->roll);
        MSG_WriteLong(&netbuffertemp, player->viewheight);
        MSG_WriteLong(&netbuffertemp, player->deltaviewheight);
        MSG_WriteLong(&netbuffertemp, player->jumpTics);
        MSG_WriteLong(&netbuffertemp, mo->reactiontime);
	}

	capture(&netbuffertemp);
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

	uint32_t len = 0;
	uint32_t tic = 0;
	size_t cnt = 0;
	cnt += sizeof(len) * fread(&len, sizeof(len), 1, demofp);
	len = LONG(len);			// convert to native byte order
	cnt += sizeof(tic) * fread(&tic, sizeof(tic), 1, demofp);
	gametic = LONG(tic);		// convert to native byte order

	char *msgdata = new char[len];
	cnt += fread(msgdata, 1, len, demofp);
	if (cnt < len + sizeof(len) + sizeof(gametic))
	{
		error("Can not read netdemo message.");
		return;
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
		CL_ReadPacketHeader();
		CL_ParseCommands();
		CL_SaveCmd();
		if (gametic - last_received > 65)
		{
			noservermsgs = true;
		}
	}
}


//
// capture()
//
// Copies data from netbuffer to the recording file
//

void NetDemo::capture(const buf_t* netbuffer)
{
	if (!isRecording())
	{
		return;
	}

	if (gamestate == GS_DOWNLOAD)
	{
		// I think this will skip the downloading process
		return;
	}

	// captures just the net packets before the game
	uint32_t len = LONG(netbuffer->cursize);
	fwrite(&len, sizeof(len), 1, demofp);
	uint32_t tic = LONG(gametic);
	fwrite(&tic, sizeof(tic), 1, demofp);

	fwrite(netbuffer->data, 1, netbuffer->cursize, demofp);
}



VERSION_CONTROL (cl_demo_cpp, "$Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $")
