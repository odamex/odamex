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

//make the storage big for the new infomation
#define SAFETYMARGIN 128


void CL_BeginNetRecord(const char* demoname)
{
	netdemoRecord = true;
	netdemoPlayback = false;

	//strcat(demoname, ".odd\0");
	//demoname.append(".odd");
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

void CL_WirteNetDemoMessages(buf_t* netbuffer, bool usercmd)
{
	//captures the net packets and local movements
	player_t* clientPlayer = &consoleplayer();
	size_t len = netbuffer->cursize;
	byte waterlevel;
	fixed_t x, y, z;
	fixed_t momx, momy, momz;
	fixed_t pitch, roll, viewheight, deltaviewheight;
	angle_t angle;
	int jumpTics, reactiontime;
	buf_t netbuffertemp;
	
	netbuffertemp.resize(len + SAFETYMARGIN);
	
	memcpy(netbuffertemp.data, netbuffer->data, netbuffer->cursize);
	netbuffertemp.cursize = netbuffer->cursize;

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
		pitch = clientPlayer->mo->pitch;
		roll = clientPlayer->mo->roll;
		viewheight = clientPlayer->viewheight;
		deltaviewheight = clientPlayer->deltaviewheight;
		jumpTics = clientPlayer->jumpTics;
		reactiontime = clientPlayer->mo->reactiontime;
		waterlevel = clientPlayer->mo->waterlevel;
	}
	/*else
		return;*/

	if(usercmd)
	{
		MSG_WriteByte(&netbuffertemp, svc_netdemocap);
		
		
		MSG_WriteByte(&netbuffertemp, clientPlayer->cmd.ucmd.buttons);

		//MSG_WriteByte(&netbuffertemp, clientPlayer->cmd.ucmd.use);
		//MSG_WriteByte(&netbuffertemp, clientPlayer->cmd.ucmd.impulse);
		MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.yaw);
		MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.forwardmove);
		MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.sidemove);
		MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.upmove);
		MSG_WriteShort(&netbuffertemp, clientPlayer->cmd.ucmd.roll);
		//MSG_WriteLong(&netbuffertemp, gametic);
		//MSG_WriteLong(&netbuffertemp, last_received);
	
		MSG_WriteByte(&netbuffertemp, waterlevel);
		MSG_WriteLong(&netbuffertemp, x);
		MSG_WriteLong(&netbuffertemp, y);
		MSG_WriteLong(&netbuffertemp, z);
		MSG_WriteLong(&netbuffertemp, momx);
		MSG_WriteLong(&netbuffertemp, momy);
		MSG_WriteLong(&netbuffertemp, momz);
		MSG_WriteLong(&netbuffertemp, angle);
		MSG_WriteLong(&netbuffertemp, pitch);
		MSG_WriteLong(&netbuffertemp, roll);
		MSG_WriteLong(&netbuffertemp, viewheight);
		MSG_WriteLong(&netbuffertemp, deltaviewheight);
		MSG_WriteLong(&netbuffertemp, jumpTics);
		MSG_WriteLong(&netbuffertemp, reactiontime);
    }

	//just going to keep this just case it is needed later - NullPoint
	/*if (clientPlayer->pendingweapon != wp_nochange)
	{
			MSG_WriteByte(&netbuffertemp, svc_changeweapon);
			MSG_WriteByte(&netbuffertemp, clientPlayer->pendingweapon);
	}*/

	CL_CaptureDeliciousPackets(&netbuffertemp);
}
	
void CL_StopRecordingNetDemo()
{
	if(!netdemoRecord)
	{
		Printf(PRINT_HIGH, "Not recording a demo no need to stop it.\n");
		return;
	}

	netdemoRecord = false;
	
	SZ_Clear(&net_message);

	MSG_WriteByte(&net_message, svc_netdemostop);

	size_t len = net_message.size();
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);
	fwrite(&gametic, sizeof(int), 1, recordnetdemo_fp);
	fwrite(net_message.data, 1, len, recordnetdemo_fp);
	
	fclose(recordnetdemo_fp);
	recordnetdemo_fp = NULL;
	Printf(PRINT_HIGH, "Demo has been stopped recording\n");
}

void CL_ReadNetDemoMeassages(buf_t* net_message)
{
	size_t len = 0;
	player_t* clientPlayer = &consoleplayer();
	
	if(!netdemoPlayback)
	{
		return;
	}
	
	/*if(netdemoPaused){
		return;
	}*/

	
	
	fread(&len, sizeof(size_t), 1, recordnetdemo_fp);
	fread(&gametic, sizeof(int), 1, recordnetdemo_fp);
	net_message->cursize = len;
	net_message->readpos = 0;
	fread(net_message->data, 1, len, recordnetdemo_fp);
 		
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

		if (gametic - last_received > 65)
		   noservermsgs = true;
	}

	
	
	SZ_Clear(net_message);
}


void CL_StopDemoPlayBack()
{
	
	if(!netdemoPlayback)
	{
		return;
	}

    SZ_Clear(&net_message);
 
    CL_QuitNetGame();
    
    Printf(PRINT_HIGH, "Demo was stopped.\n");
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

void CL_CaptureDeliciousPackets(buf_t* netbuffer)
{
	//captures just the net packets before the game
	
	if(gamestate == GS_DOWNLOAD)
	{
		//I think this will skip the downloading process
		return;
	}

	if(!netdemoRecord)
		return;
	

	size_t len = netbuffer->cursize;
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);
	fwrite(&gametic, sizeof(int), 1, recordnetdemo_fp);
	fwrite(netbuffer->data, 1, len, recordnetdemo_fp);
	//SZ_Clear(&netbuffer);
}

BEGIN_COMMAND(netrecord)
{
	std::string demonamearg;
	
	
	
	if(argc < 2)
	{
		demonamearg = "demo.odd";
	}
	else
	{
		demonamearg = argv[1];
		demonamearg.append(".odd");
	}

	/*if(netdemoRecord)
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
	}*/
	
	if(recordnetdemo_fp)
	{
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before recording a new one.\nuse the stopnetdemo command to stop the demo\n");
		return;
	}

	CL_Reconnect();
	CL_BeginNetRecord(demonamearg.c_str());
	
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
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before playing a new one.\nuse the stopnetdemo command to stop the demo\n");
		return;
	}

	if(connected)
	{
		CL_QuitNetGame();
	}

	std::string demonamearg = argv[1];

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
