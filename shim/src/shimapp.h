// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
//
// Permission is hereby granted, free of charge,
// to any person obtaining a copy of this software and associated documentation
// files(the “Software”),
// to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and / or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all copies
// or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”,
// WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
// .IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM,
// OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// DESCRIPTION:
//
// A small application that acts as a shim to link commercial libraries against,
// so that Odamex can utilize commercial SDKs like Steam, Discord etc.
// without having to link against a non-GPL library.
//
//-----------------------------------------------------------------------------

#pragma once

#include <ctime>

#define DEBUGPIPE 1
#if DEBUGPIPE
#define dbgpipe printf
#else
static inline void dbgpipe(const char* fmt, ...) { }
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
} ShimEvent;

class OShimApp
{
  public:
	static OShimApp& getInstance(void); // returns the instantiated OShimApp
	                                // object
	void Shim_ProcessLoop(void);
  private:
	OShimApp();

	int pumpcount;
	time_t pumpNotifySec;

	PipeType GPipeRead;
	PipeType GPipeWrite;

	// Last communication time.
	// If no communication within 30 seconds
	// Exit the process
	time_t lastParentCommTime;

	// Cross platform implementations
	int write1ByteCmd(const uint8_t b1);
	int write2ByteCmd(const uint8_t b1, const uint8_t b2);
	int write3ByteCmd(const uint8_t b1, const uint8_t b2, const uint8_t b3);

	bool writeBye();
	bool writeHello();

	int initPipes(void);
	void processCommands(void);
	bool processCommand(const uint8_t* buf, unsigned int buflen);
	bool checkCommunication(void);

	void Shim_deinit(void);
	int Shim_init(void);

	// Windows and Unix implementations differ
	void fail(const char* err);
	bool writePipe(const void* buf, const unsigned int _len);
	int readPipe(void* buf, const unsigned int _len);
	int pipeReady(PipeType fd);
	void closePipe(PipeType fd);
	int closeProcess(ProcessType* pid);
	char* getEnvVar(const char* key, char* buf, const size_t buflen);
};

// Shim process control
void I_ShimLoop(void);
