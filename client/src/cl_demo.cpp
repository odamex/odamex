/* This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.*/


// For this demo version I was looking at darkplaces and zdaemon 1.05
#include "cl_main.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "c_dispatch.h"
#include "d_net.h"


FILE *recordnetdemo_fp = NULL;

bool netdemoFinish = false;
//bool netdemoPaused = false;
bool netdemoRecord = false;
bool netdemoPlayback = false;

/*check if we can store the data if not make it bigger
  54 is about the size of data needed to be stored 
  it is the amount of data the commands take up roughtly*/

#define SAFETYMARGIN 50


void CL_BeginNetRecord(char* demoname)
{
	netdemoRecord = true;
	netdemoPlayback = false;

	strcat(demoname, ".odd");
	Printf(PRINT_HIGH, "Writing %s\n", demoname);

	if(recordnetdemo_fp)
	{ 
		//this is to see if it already open
		fclose(recordnetdemo_fp);
        recordnetdemo_fp = NULL;
	}

	recordnetdemo_fp = fopen(demoname, "w+b");

	if(!recordnetdemo_fp)
	{
		Printf(PRINT_HIGH, "Couldn't open the file\n");
		return;
	}

}

void CL_WirteNetDemoMessages(buf_t netbuffer)
{
	//captures the net packets and local movements
	player_t* clientPlayer = &consoleplayer();
	size_t len = netbuffer.size();
	fixed_t x, y, z, momx, momy, momz;
	angle_t angle;
	buf_t netbuffertemp;
	netbuffertemp.resize(len + SAFETYMARGIN);
	memcpy(netbuffertemp.data, netbuffer.data, netbuffer.cursize);
	netbuffertemp.cursize = netbuffer.cursize;

	if(!netdemoRecord)
		return;

	/*if(netdemoPaused)
		return;*/

	if(clientPlayer->mo)
	{
		x = clientPlayer->mo->x;
		y = clientPlayer->mo->y;
		z = clientPlayer->mo->z;
		momx = clientPlayer->mo->momx;
		momy = clientPlayer->mo->momy;
		momz = clientPlayer->mo->momz;
		angle = clientPlayer->mo->angle;
	}
	else
		return;

	MSG_WriteByte(&netbuffertemp, svc_netdemocap);
	MSG_WriteLong(&netbuffertemp, gametic);
	MSG_WriteByte(&netbuffertemp, clientPlayer->cmd.ucmd.buttons);
	MSG_WriteByte(&netbuffertemp, clientPlayer->cmd.ucmd.use);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.pitch);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.yaw);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.forwardmove);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.sidemove);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.upmove);
	MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.roll);

	MSG_WriteLong(&netbuffertemp, x);
	MSG_WriteLong(&netbuffertemp, y);
	MSG_WriteLong(&netbuffertemp, z);
	MSG_WriteLong(&netbuffertemp, momx);
	MSG_WriteLong(&netbuffertemp, momy);
	MSG_WriteLong(&netbuffertemp, momz);
	MSG_WriteLong(&netbuffertemp, angle);

	//just going to keep this just case it is needed later - NullPoint
	/*if (clientPlayer->pendingweapon != wp_nochange)
	{
			MSG_WriteByte(&netbuffertemp, svc_changeweapon);
			MSG_WriteByte(&netbuffertemp, clientPlayer->pendingweapon);
	}*/

	len = netbuffertemp.size();
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);	
	fwrite(netbuffertemp.data, 1, len, recordnetdemo_fp);
	SZ_Clear(&netbuffertemp);
}
	
void CL_StopRecordingNetDemo()
{
	if(!netdemoRecord)
	{
		Printf(PRINT_HIGH, "Not recording a demo no need to stop it.\n");
		return;
	}

	
	
	MSG_WriteByte(&net_message, svc_disconnect);
	CL_WirteNetDemoMessages(net_message);
	
	netdemoRecord = false;
	
	fclose(recordnetdemo_fp);
	recordnetdemo_fp = NULL;
	Printf(PRINT_HIGH, "Demo has been stopped recording\n");
}

void CL_ReadNetDemoMeassages()
{
	size_t len = 0;
	player_t clientPlayer = consoleplayer();
	
	if(!netdemoPlayback)
	{
		return;
	}
	
	/*if(netdemoPaused){
		return;
	}*/

	
	
	fread(&len, sizeof(size_t), 1, recordnetdemo_fp);
	net_message.resize(len);
	net_message.setcursize(len);
	net_message.readpos = 0;
	fread(net_message.data, 1, len, recordnetdemo_fp);
	
	if(!connected)
	{
		int type = MSG_ReadLong();

		if(type == CHALLENGE)
		{
			CL_PrepareConnect();
		} else if(type == 0){
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
	}
	
	SZ_Clear(&net_message);
}


void CL_StopDemoPlayBack()
{
	
	if(!netdemoPlayback)
	{
		return;
	}

	fclose(recordnetdemo_fp);
	netdemoPlayback = false;
	recordnetdemo_fp = NULL;
	gameaction = ga_fullconsole;
	gamestate = GS_FULLCONSOLE;
}

void CL_StartDemoPlayBack(std::string demoname)
{
	FixPathSeparator (demoname);
	
	if(recordnetdemo_fp)
	{ 
		//this is to see if it already open
		fclose(recordnetdemo_fp);
        recordnetdemo_fp = NULL;
	}
	
	recordnetdemo_fp = fopen(demoname.c_str(), "r+b");

	if(!recordnetdemo_fp)
	{
		Printf(PRINT_HIGH, "Couldn't open file");
		gameaction = ga_nothing;
		gamestate = GS_FULLCONSOLE;
		return;
	}


	netdemoPlayback = true;
	gamestate = GS_CONNECTING;
	netdemoRecord = false;
	netdemoFinish = false;
	//netdemoPaused = false;
}

void CL_CaptureDeliciousPackets(buf_t netbuffer)
{
	//captures just the net packets before the game
	
	if(gamestate == GS_DOWNLOAD)
	{
		//I think this will skip the downloading process
		return;
	}

	if(!netdemoRecord)
		return;

	size_t len = netbuffer.size();
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);
	fwrite(netbuffer.data, 1, len, recordnetdemo_fp);
	SZ_Clear(&netbuffer);
}

BEGIN_COMMAND(netrecord)
{
	char* demonamearg = argv[1];
	
	if(netdemoRecord)
	{
		CL_StopRecordingNetDemo();
	}

	if(connected)
	{
		CL_QuitNetGame();
	} 
	else 
	{
		Printf(PRINT_HIGH, "Need to be in a server for this to work.\n");
		return;
	}
	

	if(argc < 1)
	{
		Printf(PRINT_HIGH, "Please add a name for the demo.\n");
		return;
	}

	if(recordnetdemo_fp)
	{
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before recording a new one.\n");
		return;
	}
	
	CL_BeginNetRecord(demonamearg);
	CL_Reconnect();
}
END_COMMAND(netrecord)

/*BEGIN_COMMAND(netpuase){
	if(netdemoPaused){
		netdemoPaused = false;
	} else {
		netdemoPaused = true;
	}
}
END_COMMAND(netpuase)
*/
BEGIN_COMMAND(netplay)
{
	if(argc < 1)
	{
		Printf(PRINT_HIGH, "Usage: netplay <demoname>\n");
		return;
	}

	if(recordnetdemo_fp)
	{
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before playing a new one.\n");
		return;
	}

	if(connected)
	{
		CL_QuitNetGame();
	}

	char* demonamearg = argv[1];

	CL_StartDemoPlayBack(demonamearg);

}
END_COMMAND(netplay)

BEGIN_COMMAND(stopnetdemo)
{
	if(netdemoRecord)
	{
		CL_StopRecordingNetDemo();
	} 
	else if(netdemoPlayback)
	{
		CL_StopDemoPlayBack();
	}
}
END_COMMAND(stopnetdemo)
