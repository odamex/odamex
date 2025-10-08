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

#include <string>
#include <sstream>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1 // we don't need the gui
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

#include "shimapp.h"
#include "discordlib.h"

OShimApp::OShimApp()
{
	GPipeRead = NULLPIPE;
	GPipeWrite = NULLPIPE;
	lastParentCommTime = 0;
	pumpcount = 0;
	pumpNotifySec = 0;
}

OShimApp& OShimApp::getInstance()
{
	static OShimApp instance;
	return instance;
}

int OShimApp::write1ByteCmd(const uint8_t b1)
{
	uint16_t length = 1;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1};
	return writePipe(buf, sizeof(buf));
}

int OShimApp::write2ByteCmd(const uint8_t b1, const uint8_t b2)
{
	uint16_t length = 2;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1, b2};
	return writePipe(buf, sizeof(buf));
}

int OShimApp::write3ByteCmd(const uint8_t b1, const uint8_t b2, const uint8_t b3)
{
	uint16_t length = 3;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1, b2};
	return writePipe(buf, sizeof(buf));
}

bool OShimApp::writeBye()
{
	dbgpipe("Child sending SHIMEVENT_BYE().\n");
	return write1ByteCmd(SHIMEVENT_BYE);
}

bool OShimApp::writeHello()
{
	dbgpipe("Child sending SHIMEVENT_HELLO().\n");
	return write1ByteCmd(SHIMEVENT_HELLO);
}

#ifdef _WIN32

int OShimApp::pipeReady(PipeType fd)
{
	DWORD avail = 0;
	return (PeekNamedPipe(fd, NULL, 0, NULL, &avail, NULL) && (avail > 0));
}

void OShimApp::fail(const char* err)
{
	MessageBoxA(NULL, err, "ERROR", MB_ICONERROR | MB_OK);
	ExitProcess(1);
}

bool OShimApp::writePipe(const void* buf, const unsigned int _len)
{
	const DWORD len = (DWORD)_len;
	DWORD bw = 0;
	return ((WriteFile(GPipeWrite, buf, len, &bw, NULL) != 0) && (bw == len));
}

int OShimApp::readPipe(void* buf, const unsigned int _len)
{
	const DWORD len = (DWORD)_len;
	DWORD br = 0;
	return ReadFile(GPipeRead, buf, len, &br, NULL) ? (int)br : -1;
}

void OShimApp::closePipe(PipeType fd)
{
	CloseHandle(fd);
}

int OShimApp::closeProcess(ProcessType* pid)
{
	CloseHandle(pid->hProcess);
	CloseHandle(pid->hThread);
	return 0;
}

char* OShimApp::getEnvVar(const char* key, char* buf, const size_t _buflen)
{
	const DWORD buflen = (DWORD)_buflen;
	const DWORD rc = GetEnvironmentVariableA(key, buf, buflen);
	// rc doesn't count null char, hence "<".
	return ((rc > 0) && (rc < buflen)) ? buf : NULL;
}

#else

#if !defined(POLLIN)

#define POLLIN 0x01
#define POLLPRI 0x02
#define POLLOUT 0x04
#define POLLERR 0x08
#define POLLHUP 0x10
#define POLLNVAL 0x20

struct pollfd
{
	PipeType fd;
	short events;
	short revents;
};

#endif

int OShimApp::pipeReady(PipeType fd)
{
	int rc;
	struct pollfd pfd = {fd, POLLIN | POLLERR | POLLHUP, 0};
	while (((rc = poll(&pfd, 1, 0)) == -1) && (errno == EINTR))
	{
	}
	return (rc == 1);
}

void OShimApp::fail(const char* err)
{
	fprintf(stderr, "%s\n", err);
	_exit(1);
}

bool OShimApp::writePipe(const void* buf, const unsigned int _len)
{
	const ssize_t len = (ssize_t)_len;
	ssize_t bw;
	while (((bw = write(GPipeWrite, buf, len)) == -1) && (errno == EINTR))
	{
	}
	return (bw == len);
}

int OShimApp::readPipe(void* buf, const unsigned int _len)
{
	const ssize_t len = (ssize_t)_len;
	ssize_t br;
	while (((br = read(GPipeRead, buf, len)) == -1) && (errno == EINTR))
	{
	}
	return (int)br;
}

void OShimApp::closePipe(PipeType fd)
{
	close(fd);
}

int OShimApp::closeProcess(ProcessType* pid)
{
	int rc = 0;
	while ((waitpid(*pid, &rc, 0) == -1) && (errno == EINTR))
	{
	}
	if (!WIFEXITED(rc))
		return 1; // oh well.
	return WEXITSTATUS(rc);
}

char* OShimApp::getEnvVar(const char* key, char* buf, const size_t buflen)
{
	const char* envr = getenv(key);
	if (!envr || (strlen(envr) >= buflen))
		return NULL;
	strcpy(buf, envr);
	return buf;
}

#endif


int OShimApp::initPipes(void)
{
	char buf[64];
	unsigned long long val;

	if (!getEnvVar("SHIM_READHANDLE", buf, sizeof(buf)))
		return 0;
	else if (sscanf(buf, "%llu", &val) != 1)
		return 0;
	else
		GPipeRead = (PipeType)val;

	if (!getEnvVar("SHIM_WRITEHANDLE", buf, sizeof(buf)))
		return 0;
	else if (sscanf(buf, "%llu", &val) != 1)
		return 0;
	else
		GPipeWrite = (PipeType)val;

	return ((GPipeRead != NULLPIPE) && (GPipeWrite != NULLPIPE));
}

int OShimApp::Shim_init(void)
{
	dbgpipe("Child shim init start.\n");
	if (!initPipes())
	{
		dbgpipe("Child shim init failed.\n");
		return 0;
	}

	// signal(SIGPIPE, SIG_IGN);

	dbgpipe("Child shim init success!\n");
	return 1;
}

void OShimApp::Shim_deinit(void)
{
	dbgpipe("Child deinit.\n");
	if (GPipeWrite != NULLPIPE)
	{
		writeBye();
		closePipe(GPipeWrite);
	}

	if (GPipeRead != NULLPIPE)
		closePipe(GPipeRead);

	GPipeRead = GPipeWrite = NULLPIPE;

	// signal(SIGPIPE, SIG_DFL);
}

bool OShimApp::processCommand(const uint8_t* buf, unsigned int buflen)
{
	if (buflen == 0)
		return true;

	const ShimCmd cmd = (ShimCmd) * (buf++);
	buflen--;

#if DEBUGPIPE
	if (false)
	{
	}
#define PRINTGOTCMD(x) else if (cmd == x) printf("Child got " #x ".\n")
	PRINTGOTCMD(SHIMCMD_BYE);
	PRINTGOTCMD(SHIMCMD_HELLO);
	//PRINTGOTCMD(SHIMCMD_PUMP);
	else if (cmd == SHIMCMD_PUMP)
	{
		time_t cur = std::time(0);
		if (cur % 5 == 0 && pumpNotifySec != cur)
		{
			printf("Child got %d SHIMCMD_PUMPCOUNT in 5 seconds.\n", pumpcount);
			pumpcount = 0;
			pumpNotifySec = cur;
		}
	}
	PRINTGOTCMD(SHIMCMD_SETSTATUS);
#undef PRINTGOTCMD
	else printf("Child got unknown shimcmd %d.\n", (int)cmd);
#endif

	lastParentCommTime = std::time(0);

	switch (cmd)
	{
	case SHIMCMD_PUMP:
		ODiscordLib::getInstance().Run_Callbacks();
		pumpcount++;
		break;

	case SHIMCMD_HELLO:
		// send back a hello event
		writeHello();
		break;

	case SHIMCMD_BYE:
		// Terminate
		return false;
	break;

	case SHIMCMD_SETSTATUS:
		if (buflen)
		{
			StatusUpdate item = {};
			item.status_update_deserialize(buf, buflen);

			ODiscordLib::getInstance().Update_RichPresence(item);
		}
	break;
	}

	return true;
}

bool OShimApp::checkCommunication()
{
	time_t curTime = std::time(0);
	time_t secSinceLast = curTime - lastParentCommTime;

	if (secSinceLast > 0 && secSinceLast % 30 == 0)
	{
		dbgpipe("OdaShim: Exiting due to no communication for 30 seconds.\n");
		return false;
	}

	return true;
}

void OShimApp::processCommands()
{
	bool quit = false;
	static uint8_t buf[65536];
	int br;

	// this read blocks.
	while (!quit)
	{
		if (!checkCommunication())
		{
			quit = true;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		if (!pipeReady(GPipeRead))
		{
			continue;
		}

		br = readPipe(buf, sizeof(buf));

		while (br > 0)
		{
			int cmdlen;
			const int rawdatalength = br - 2;

			if (rawdatalength >= 0 &&
			    rawdatalength >= (cmdlen = *reinterpret_cast<uint16_t*>(buf)))
			{
				if (!processCommand(buf + 2, cmdlen))
				{
					quit = true;
					break;
				}

				br -= cmdlen + 2;
				if (br > 0)
					memmove(buf, buf + cmdlen + 2, br);
			}
			else // get more data.
			{
				const int morebr = readPipe(buf + br, sizeof(buf) - br);
				if (morebr <= 0)
				{
					quit = true; // uhoh.
					break;
				}
				br += morebr;
			}
		}
	}
}

void OShimApp::Shim_ProcessLoop(void)
{
	if (!Shim_init())
	{
		dbgpipe("OdaShim: Something went wrong with initializing the shim!\n");
	}
	dbgpipe("OdaShim: Entering command loop.\n");

	lastParentCommTime = std::time(0);

	ODiscordLib::getInstance().Init_DiscordSdk();

	processCommands();

	Shim_deinit();
}

void I_ShimLoop(void)
{
	OShimApp::getInstance().Shim_ProcessLoop();
}

#ifdef _WIN32

int __cdecl main(int argc, char* argv[])
{
	I_ShimLoop();
	ExitProcess(0);
	return 0;
}

#else

static int GArgc = 0;
static char** GArgv = NULL;

int main(int argc, char** argv)
{
	signal(SIGPIPE, SIG_IGN);
	GArgc = argc;
	GArgv = argv;
	I_ShimLoop();
	return 0;
}

#endif
