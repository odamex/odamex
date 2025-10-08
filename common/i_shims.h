// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//
// A singleton class containing the IPC functionality with the
// main shim process that handles all functionality with commercial libraries.
// 
// e.g. steamworks, epic online services, discord sdk
// 
// This is so Odamex doesn't have to link against any non-GPL libraries.
// Instead, the libraries are linked against the shim process, which is licensed as MIT
// 
// Odamex runs this shim executable as a child process, sets up pipe communication to it,
// and then transmits events that their respective library subscribes to, and takes action
// when that message applies to them.
// 
// If this service isn't running, Odamex will check for it 5 times at certain intervals.
// If the child process cannot process the pipes, it finally backs off and warns to console.
// 
// This class also handles communication with bootstrappers, as in external programs
// that launch Odamex as a child process, like the Steam Bootstrapper or the
// Epic Games bootstrapper.
//
//-----------------------------------------------------------------------------

#pragma once

#include "g_status.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1 // we don't need the gui
#endif
#include <windows.h>
typedef PROCESS_INFORMATION ProcessType;
typedef HANDLE PipeType;
#define NULLPIPE NULL
#else
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
typedef pid_t ProcessType;
typedef int PipeType;
#define NULLPIPE -1
#endif

typedef enum ShimCmd
{
	SHIMCMD_BYE,
	SHIMCMD_HELLO,
	SHIMCMD_PUMP,
	SHIMCMD_SETSTATUS
} ShimCmd;

typedef enum ShimEventType
{
	SHIMEVENT_BYE,
	SHIMEVENT_HELLO,
} ShimEventType;

struct ShimEvent
{
	ShimEventType type;
	int okay;
	int ivalue;
	float fvalue;
	unsigned long long epochsecs;
	char name[256];
};

class OShim
{
  public:
	static OShim& getInstance();       // returns the instantiated OShim
	                                   // object
	void Shim_initialize(void);
	void Shim_deinitialize(void);
	void Shim_sendStatusUpdate(const StatusUpdate& update);
	void Shim_tick(void);
	const ShimEvent* Shim_pump(void);
	const StatusUpdate& GetLastStatus();
	time_t GetLastStatusTime();

  private:
	OShim();
	// Process Control
	
	// Backoff trying to restart the shim process
	bool backoff;

	// Time of the last communication event with the shim
	// After 45 seconds of no communication, attempt to restart
	// Every 5 seconds, send a hello
	time_t lastWriteTime;

	// Last time we sent a hello
	time_t lastHelloTime;

	// Current PID of the shim launched if detected in the same directory
	ProcessType shimPid;

	/////////////////////////////

	// Child Process Shim
	PipeType PParentRead;
	PipeType PParentWrite;

	// Bootstrapper Process Shim
	PipeType BParentRead;
	PipeType BParentWrite;

	// Status update send control
	time_t lastStatusTime;

	StatusUpdate lastStatus;

	// Cross platform implementations
	const ShimEvent* processEvent(const uint8_t* buf, size_t buflen);
	bool write1ByteCmd(PipeType fd, const uint8_t b1);
	bool write2ByteCmd(PipeType fd, const uint8_t b1, const uint8_t b2);
	bool write3ByteCmd(PipeType fd, const uint8_t b1, const uint8_t b2, const uint8_t b3);
	int writeBye(PipeType fd);
	int writeHello(PipeType fd);
	int isAlive(PipeType fd);
	int isDead(PipeType fd);
	bool setEnvironmentVars(PipeType pipeChildRead, PipeType pipeChildWrite);
	void fail(const char* err);

	// Windows and Unix implementations differ
	bool writePipe(PipeType fd, const void* buf, const unsigned int _len);
	int readPipe(PipeType fd, void* buf, const unsigned int _len);
	bool createPipes(PipeType* pPipeParentRead, PipeType* pPipeParentWrite,
	                 PipeType* pPipeChildRead, PipeType* pPipeChildWrite);
	void closePipe(PipeType fd);
	int pipeReady(PipeType fd);
	int initBootstrapperPipes(void);

	char* getEnvVar(const char* key, char* buf, const size_t buflen);
	bool setEnvVar(const char* key, const char* val);
	bool launchChildShim(ProcessType* pid);
	int closeChildProcess(ProcessType* pid);

	// Shim communication functions
	void sendStatusUpdate(const StatusUpdate& update);
};

// Shim Process Functions
void I_InitShim(void);
void I_ShutdownShim(void);
void I_GameStatusTicker(const StatusUpdate& update);
const ShimEvent* I_ProcessShim(void);
