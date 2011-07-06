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

#include "cl_main.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "c_dispatch.h"
#include "d_net.h"
#include "cl_demo.h"
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
	memcpy(&to.header, &from.header, sizeof(netdemo_header_t));
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

	memset(&header, 0, sizeof(header));
	strncpy(header.identifier, "ODAD", 4);
	header.version = NETDEMOVER;
	header.compression = 0;

	// Note: The header is not finalized at this point.  Write it anyway to
	// reserve space in the output file for it and overwrite it later.
	if (!fwrite(&header, sizeof(header), 1, demofp))
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
		error("Unable to open netdemo file.\n");
		return false;
	}

    // read the demo's header file
    if (fread(&header, 1, sizeof(header), demofp) < sizeof(header) ||
        strncmp(header.identifier, "ODAD", 4) != 0)
    {
        error("Unable to read netdemo header.\n");
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
	
	if (fread(&index, header.index_size, 1, demofp) < (unsigned)header.index_size)
	{
		error("Unable to read netdemo index.\n");
		return false;
	}

	// get set up to read server cmds
	fseek(demofp, sizeof(header), SEEK_SET);
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

	// flush the netbuffer
    SZ_Clear(&net_message);

	// write the end-of-demo marker
	MSG_WriteByte(&net_message, svc_netdemostop);
	size_t len = net_message.size();
	fwrite(&len, sizeof(size_t), 1, demofp); 
	fwrite(&gametic, sizeof(int), 1, demofp);
	fwrite(net_message.data, 1, len, demofp);

	// tack the index onto the end of the recording
	header.index_offset = ftell(demofp);
	header.index_size = index.size() * sizeof(netdemo_index_entry_t);
	for (size_t i = 0; i < index.size(); i++)
	{
		fwrite(&index[i], sizeof(netdemo_index_entry_t), 1, demofp);
	}

	// rewrite the header since index_offset and index_size is now known
	fseek(demofp, 0, SEEK_SET);
	fwrite(&header, 1, sizeof(header), demofp);

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

	size_t len = 0;
	fread(&len, sizeof(size_t), 1, demofp);
	fread(&gametic, sizeof(int), 1, demofp);

	char *msgdata = new char[len];
	fread(msgdata, 1, len, demofp);
	netbuffer->WriteChunk(msgdata, len);
	delete msgdata;

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
	size_t len = netbuffer->cursize;
	fwrite(&len, sizeof(size_t), 1, demofp);
	fwrite(&gametic, sizeof(int), 1, demofp);
	fwrite(netbuffer->data, 1, len, demofp);
}



VERSION_CONTROL (cl_demo_cpp, "$Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $")
