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
// If the child process cannot process the pipes, it finally backs off and warns to
// console.
// 
// This class also handles communication with bootstrappers, as in external programs
// that launch Odamex as a child process, like the Steam Bootstrapper or the
// Epic Games bootstrapper.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <string>

#include <sstream>
#include <locale>

#include "i_shims.h"

#define DEBUGPIPE 0

std::string M_GetBinaryDir();

OShim::OShim()
{
	PParentRead = NULLPIPE;
	PParentWrite = NULLPIPE;
	BParentRead = NULLPIPE;
	BParentWrite = NULLPIPE;

	backoff = false;
	lastWriteTime = 0;
	lastHelloTime = 0;
	lastStatusTime = 0;

#ifdef _WIN32
	shimPid = {0, 0, 0, 0};
#else
	shimPid = 0;
#endif
}

OShim& OShim::getInstance()
{
	static OShim instance;
	return instance;
}

void OShim::Shim_tick(void)
{
	if (backoff)
		return;

	if (isDead(PParentRead) && isDead(PParentWrite))
	{
		Shim_initialize();
		return;
	}

	time_t curtime = std::time(0);
	time_t timediff = curtime - lastWriteTime;

	if (timediff <= 0)
		return;

	if (timediff % 5 == 0 && lastHelloTime != curtime)
	{
		// Every 5+ seconds of no communication, send a hello
		writeHello(PParentWrite);
	}

	if (timediff % 45 == 0)
	{
		// After 45 seconds of no communication,
		// We can assume the shim is no longer active.
		// Relaunch it, the shim this interacts with
		// SHOULD self terminate
		Printf(PRINT_WARNING, "I_Shim: Communication with shim last happened %d seconds ago.\n", timediff);
		Printf(PRINT_HIGH, "I_Shim: Attempting to clean up and restart shim.\n", timediff);
		writeBye(PParentWrite);
		Shim_deinitialize();
		Shim_initialize();
	}
}

bool OShim::setEnvironmentVars(PipeType pipeChildRead, PipeType pipeChildWrite)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%llu", (unsigned long long)pipeChildRead);
	if (!setEnvVar("SHIM_READHANDLE", buf))
		return false;

	snprintf(buf, sizeof(buf), "%llu", (unsigned long long)pipeChildWrite);
	if (!setEnvVar("SHIM_WRITEHANDLE", buf))
		return false;

	return true;
}

bool OShim::write1ByteCmd(PipeType fd, const uint8_t b1)
{
	uint16_t length = 1;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1};
	return writePipe(fd, buf, sizeof(buf));
}

bool OShim::write2ByteCmd(PipeType fd, const uint8_t b1, const uint8_t b2)
{
	uint16_t length = 2;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1, b2};
	return writePipe(fd, buf, sizeof(buf));
}

bool OShim::write3ByteCmd(PipeType fd, const uint8_t b1, const uint8_t b2,
                             const uint8_t b3)
{
	uint16_t length = 3;
	uint8_t* length_bytes = (uint8_t*)&length;
	const uint8_t buf[] = {length_bytes[0], length_bytes[1], b1, b2, b3};
	return writePipe(fd, buf, sizeof(buf));
}

int OShim::writeBye(PipeType fd)
{
	//Printf(PRINT_HIGH, "I_Shim: Sending SHIMCMD_BYE() to shim.\n");
	return write1ByteCmd(fd, SHIMCMD_BYE);
}

int OShim::writeHello(PipeType fd)
{
	//Printf(PRINT_HIGH, "I_Shim: Sending SHIMCMD_HELLO() to shim.\n");
	lastHelloTime = std::time(0);
	return write1ByteCmd(fd, SHIMCMD_HELLO);
}

int OShim::isAlive(PipeType fd)
{
	return (fd != NULLPIPE);
}

int OShim::isDead(PipeType fd)
{
	return !isAlive(fd);
}

void OShim::fail(const char* err)
{
	Printf(PRINT_WARNING,
	       "I_Shim: Cannot run shim -- attempt to run exited with error: %s\n", err);
	Printf(PRINT_WARNING,
	       "This generally happens because odashim isn't in the same directory as Odamex!\n");
}

int OShim::initBootstrapperPipes(void)
{
	char buf[64];
	unsigned long long val;

	if (!getEnvVar("BOOTSTRAPPER_READHANDLE", buf, sizeof(buf)))
		return 0;
	else if (sscanf(buf, "%llu", &val) != 1)
		return 0;
	else
		BParentRead = (PipeType)val;

	if (!getEnvVar("BOOTSTRAPPER_WRITEHANDLE", buf, sizeof(buf)))
		return 0;
	else if (sscanf(buf, "%llu", &val) != 1)
		return 0;
	else
		BParentWrite = (PipeType)val;

	return ((BParentRead != NULLPIPE) && (BParentWrite != NULLPIPE));
}


void OShim::sendStatusUpdate(const StatusUpdate& update)
{
	if (isDead(PParentRead) || isDead(PParentWrite))
		return;

	//Printf("Parent sending SHIMCMD_SETSTATUS.\n");

	std::ostringstream status;

	update.status_update_serialize(status);

	std::string serialized = status.str();

	std::ostringstream data_stream;

	data_stream << (uint8_t)SHIMCMD_SETSTATUS << serialized;

	std::ostringstream data_stream_shim;

	uint16_t length = data_stream.str().length();
	data_stream_shim.write(reinterpret_cast<const char*>(&length), sizeof(length));
	data_stream_shim << data_stream.str();

	std::string buffer = data_stream_shim.str();
	writePipe(PParentWrite, buffer.data(), buffer.length());
}

#ifdef _WIN32

#include <codecvt>

static LPWSTR LpCmdLine = NULL;

int OShim::pipeReady(PipeType fd)
{
	DWORD avail = 0;
	return (PeekNamedPipe(fd, NULL, 0, NULL, &avail, NULL) && (avail > 0));
}

bool OShim::writePipe(PipeType fd, const void* buf, const unsigned int _len)
{
	const DWORD len = (DWORD)_len;
	DWORD bw = 0;
	return ((WriteFile(fd, buf, len, &bw, NULL) != 0) && (bw == len));
}

int OShim::readPipe(PipeType fd, void* buf, const unsigned int _len)
{
	const DWORD len = (DWORD)_len;
	DWORD br = 0;
	return ReadFile(fd, buf, len, &br, NULL) ? (int)br : -1;
}

bool OShim::createPipes(PipeType* pPipeParentRead, PipeType* pPipeParentWrite,
                        PipeType* pPipeChildRead, PipeType* pPipeChildWrite)
{
	SECURITY_ATTRIBUTES pipeAttr;

	pipeAttr.nLength = sizeof(pipeAttr);
	pipeAttr.lpSecurityDescriptor = NULL;
	pipeAttr.bInheritHandle = TRUE;
	if (!CreatePipe(pPipeParentRead, pPipeChildWrite, &pipeAttr, 0))
		return 0;

	pipeAttr.nLength = sizeof(pipeAttr);
	pipeAttr.lpSecurityDescriptor = NULL;
	pipeAttr.bInheritHandle = TRUE;
	if (!CreatePipe(pPipeChildRead, pPipeParentWrite, &pipeAttr, 0))
	{
		CloseHandle(*pPipeParentRead);
		CloseHandle(*pPipeChildWrite);
		return 0;
	}

	return 1;
}

void OShim::closePipe(PipeType fd)
{
	CloseHandle(fd);
}

bool OShim::setEnvVar(const char* key, const char* val)
{
	return (SetEnvironmentVariableA(key, val) != 0);
}

bool OShim::launchChildShim(ProcessType* pid)
{
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
#if !DEBUGPIPE
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
#endif
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring exe = converter.from_bytes(M_GetBinaryDir() + "\\odashim.exe");

	std::wstring args =
	    L"\"" + exe + L"\" " +
	    (LpCmdLine ? LpCmdLine : L"");

	return (CreateProcessW(&exe[0], &args[0], NULL, NULL, TRUE, 0, NULL,
	                       NULL, &si, pid) != 0);
}

int OShim::closeChildProcess(ProcessType* pid)
{
	CloseHandle(pid->hProcess);
	CloseHandle(pid->hThread);
	return 0;
}

char* OShim::getEnvVar(const char* key, char* buf, const size_t _buflen)
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

int OShim::pipeReady(PipeType fd)
{
	int rc;
	struct pollfd pfd = {fd, POLLIN | POLLERR | POLLHUP, 0};
	while (((rc = poll(&pfd, 1, 0)) == -1) && (errno == EINTR))
	{
	}
	return (rc == 1);
}

bool OShim::writePipe(PipeType fd, const void* buf, const unsigned int _len)
{
	const ssize_t len = (ssize_t)_len;
	ssize_t bw;
	while (((bw = write(fd, buf, len)) == -1) && (errno == EINTR))
	{
	}
	return (bw == len);
}

int OShim::readPipe(PipeType fd, void* buf, const unsigned int _len)
{
	const ssize_t len = (ssize_t)_len;
	ssize_t br;
	while (((br = read(fd, buf, len)) == -1) && (errno == EINTR))
	{
	}
	return (int)br;
}

bool OShim::createPipes(PipeType* pPipeParentRead, PipeType* pPipeParentWrite,
                        PipeType* pPipeChildRead, PipeType* pPipeChildWrite)
{
	int fds[2];
	if (pipe(fds) == -1)
		return 0;
	*pPipeParentRead = fds[0];
	*pPipeChildWrite = fds[1];

	if (pipe(fds) == -1)
	{
		close(*pPipeParentRead);
		close(*pPipeChildWrite);
		return 0;
	} // if

	*pPipeChildRead = fds[0];
	*pPipeParentWrite = fds[1];

	return 1;
} // createPipes

void OShim::closePipe(PipeType fd)
{
	close(fd);
} // closePipe

bool OShim::setEnvVar(const char* key, const char* val)
{
	return (setenv(key, val, 1) != -1);
} // setEnvVar

static int GArgc = 0;
static char** GArgv = NULL;

bool OShim::launchChildShim(ProcessType* pid)
{
	*pid = fork();
	if (*pid == -1) // failed
		return false;
	else if (*pid != 0) // we're the parent
		return true;    // we'll let the pipe fail if this didn't work.

		// i'm the baby!
#ifdef __APPLE__
	std::string app = M_GetBinaryDir() + "\\odashim.app";
#else
	std::string app = M_GetBinaryDir() + "\\odashim";
#endif

	GArgv[0] = strdup(M_GetBinaryDir().c_str());
	execvp(GArgv[0], GArgv);
	_exit(1);
}

int OShim::closeChildProcess(ProcessType* pid)
{
	int rc = 0;
	while ((waitpid(*pid, &rc, 0) == -1) && (errno == EINTR))
	{
	}
	if (!WIFEXITED(rc))
		return 1;
	return WEXITSTATUS(rc);
}

char* OShim::getEnvVar(const char* key, char* buf, const size_t buflen)
{
	const char* envr = getenv(key);
	if (!envr || (strlen(envr) >= buflen))
		return NULL;
	strcpy(buf, envr);
	return buf;
}

#endif

const ShimEvent* OShim::processEvent(const uint8_t* buf, size_t buflen)
{
	static ShimEvent event;
	const ShimEventType type = (ShimEventType) * (buf++);
	buflen--;

	memset(&event, '\0', sizeof(event));
	event.type = type;
	event.okay = 1;

#if DEBUGPIPE
	if (0)
	{
	}
#define PRINTGOTEVENT(x) else if (type == x) printf("I_Shim: Client got " #x ".\n")
	PRINTGOTEVENT(SHIMEVENT_BYE);
	PRINTGOTEVENT(SHIMEVENT_HELLO);
#undef PRINTGOTEVENT
	else printf("I_Shim: Client got unknown shimeventtype %d.\n", (int)type);
#endif

	lastWriteTime = std::time(0);

	switch (type)
	{
	case SHIMEVENT_BYE:
		Printf(PRINT_WARNING, "I_Shim: Shim sent BYE event, this may indicate it has exited.\n");
		break;
	case SHIMEVENT_HELLO:
			// Do nothing but record the time
		break;

	default: /* uh oh */
		return NULL;
	}

	return &event;
}

//
// Shim_Pump
//
// Pump events that the shim is attempting to tell the client.
// Run this until null or error per frame.
//

const ShimEvent* OShim::Shim_pump(void)
{
	static uint8_t buf[65536];
	static int br = 0;
	int evlen = (br > 1) ? *reinterpret_cast<uint16_t*>(buf) : 0;

	if (isDead(PParentRead) || isDead(PParentWrite) || backoff)
		return NULL;

	if (br - 2 < evlen) // we have an incomplete commmand. try to read more.
	{
		if (pipeReady(PParentRead))
		{
			const int morebr = readPipe(PParentRead, buf + br, sizeof(buf) - br);
			if (morebr > 0)
				br += morebr;
			else /* uh oh */
			{
				Printf("I_Shim: Client readPipe failed! Shutting down.\n");
				I_ShutdownShim();
			}
		}
	}

	if (evlen && (br - 2 >= evlen))
	{
		const ShimEvent* retval = processEvent(buf + 2, evlen);
		br -= evlen + 2;
		if (br > 0)
			memmove(buf, buf + evlen + 2, br);
		return retval;
	}

	// run callback event loop.
	if (br == 0)
	{
		//dbgpipe("Parent sending SHIMCMD_PUMP().\n");
		write1ByteCmd(PParentWrite, SHIMCMD_PUMP);
	}

	return NULL;
}

void OShim::Shim_initialize(void)
{
	if (backoff)
	{
		fail("Backoff issued");
		return;
	}
	PipeType PChildRead;
	PipeType PChildWrite;

	if (!createPipes(&PParentRead, &PParentWrite, &PChildRead, &PChildWrite))
		fail("Failed to create application pipes");
	else if (!setEnvironmentVars(PChildRead, PChildWrite))
		fail("Failed to set environment variables");
	else if (!launchChildShim(&shimPid))
	{
#ifdef _WIN32
		int error = GetLastError();
#else
		int error = errno;
#endif
		char str[100];
		snprintf(str, 100, "shim launch error code %d", error);
		fail(str);
		backoff = true;
	}

	// Close the ends of the pipes that the child will use; we don't need them.
	closePipe(PChildRead);
	closePipe(PChildWrite);
	PChildRead = PChildWrite = NULLPIPE;
	lastWriteTime = std::time(0); // Even though we haven't contacted yet, start the timer
}

void OShim::Shim_sendStatusUpdate(const StatusUpdate& update)
{
	sendStatusUpdate(update);
	lastStatus = update;
	lastStatusTime = std::time(0);
}

const StatusUpdate& OShim::GetLastStatus(void)
{
	return lastStatus;
}

time_t OShim::GetLastStatusTime(void)
{
	return lastStatusTime;
}

void OShim::Shim_deinitialize(void)
{
	// Close our ends of the pipes.
	writeBye(PParentWrite);
	closePipe(PParentRead);
	closePipe(PParentWrite);

	// Wait for the child to terminate, close the child process handles.
	const int retval = closeChildProcess(&shimPid);
}

//
// I_InitShim
//
// Initializes the shim pipes, sets the process envvars,
// and launches the child shim process.
//
void I_InitShim(void)
{
	Printf("I_InitShim: Initialize Commercial Shims.\n");

	OShim::getInstance().Shim_initialize();

	Printf("I_InitShim: Shim spawning successful!\n");
}

//
// I_GameStatusTicker
//
// Main ticker for in-game status updates
//
void I_GameStatusTicker(const StatusUpdate& update)
{
	time_t curtime = std::time(0);

	if (OShim::getInstance().GetLastStatusTime() == curtime)
	{
		return;
	}

	const StatusUpdate& s = OShim::getInstance().GetLastStatus();

	if (&update != &s)
	{
		OShim::getInstance().Shim_sendStatusUpdate(update);
	}
}


//
// I_ShutdownShim
//
// Sends a goodbye to the shim, waits for it to shutdown, and returns the exit code.
//
void I_ShutdownShim()
{
	Printf("I_ShutdownShim: shutting down.\n");

	OShim::getInstance().Shim_deinitialize();
}

//
// I_ProcessShim
//
// Main process loop for shims.
//
const ShimEvent* I_ProcessShim()
{
	return OShim::getInstance().Shim_pump();
}
