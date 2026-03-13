// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
// Copyright (C) 2025-2026 by The Odamex Team.
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
//  These routines provide console commands streamed in from stdin
//  or a given file.
//
//-----------------------------------------------------------------------------

#include "m_consolecommandstream.h"

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>

//  IMPORTANT WIN32 NOTE:
//
//  We used to use the standard C++ threading and sync objects for Win32, but
//  because we're forced to use TerminateThread on Win32, which, unlike POSIX,
//  leaves the mutex in a locked state, leading to undefined behavior when
//  destructed.  On Debug builds, this raises an Abort signal.  By switching to
//  SRW Locks, which don't even have a destructor, we can destruct at any time.

#endif

namespace {

	class CommandStreamReader
	{
		public:

			// Please note that we force the use of a Singleton because that's the best
			// way to ensure that the thread, the file handles, and the other resources
			// are created in the correct order and cleaned up during runtime teardown.
			//
			//  1.  The purpose of the thread is to block on an istream without interrupting
			//      the main program.
			//
			//  2.  The C++ standard does NOT guarantee any way to safely, portably interrupt
			//      the blocking read when using stdin, therefore we essentially cannot
			//      intelligently interrupt the thread; We can cancel/terminate it, leave the
			//      sync objects in an undefined state, and let the runtime clean this all up
			//      during teardown.
			//
			//  3.  If we allow this object to be destructed directly, and we have no way of
			//      gracefully ending and joining the thread, then we can easily wind up with
			//      a thread that attempts to access freed resources (i.e. member variables).
			//
			// The best way to address this is to just make a Meyers Singleton and be done
			// with it.
			static CommandStreamReader& Get()
			{
				static CommandStreamReader instance(s_filePtr ? *s_filePtr : std::cin);
				return instance;
			}

			// Set the contents of the given string, an output parameter, to the next command
			// received from the command stream.  If a command is available, the given string
			// is updated and true is returned.  Otherwise, the string is unmodified and false
			// is returned.
			//
			// Meant to be called from the main thread.
#ifdef _WIN32
			bool GetCommand(std::string& o_command)
			{
				if (TryAcquireSRWLockExclusive(& m_srwLock))
				{
					if (not m_command.empty())
					{
						o_command.swap(m_command);
						m_command.clear();

						// Unlock early so that we can be sure that the thread doesn't awake via
						// the condition variable then turn around and immediately have to wait
						// on the mutex.
						ReleaseSRWLockExclusive(& m_srwLock);
						WakeConditionVariable(& m_commandIsReleasedCondition);
						return true;
					}
					ReleaseSRWLockExclusive(& m_srwLock);
				}
				return false;
			}
#else
			bool GetCommand(std::string& o_command)
			{
				if (std::unique_lock lock{m_mutex, std::try_to_lock})
				{
					if (not m_command.empty())
					{
						o_command.swap(m_command);
						m_command.clear();

						// Unlock early so that we can be sure that the thread doesn't awake via
						// the condition variable then turn around and immediately have to wait
						// on the mutex.
						lock.unlock();
						m_commandIsReleasedCondition.notify_one();
						return true;
					}
				}
				return false;
			}
#endif

			// Commit to using the given Input File for the CommandStream instead of stdin.  This
			// must be done before the first attempt to access the CommandStream, and can only be
			// completed successfully once.  If the file is successfully opened, then true is
			// returned, and all subsequent calls to this function do nothing and return false.
			static bool OpenInputFile(const char* filepath)
			{
				if (filepath and not s_filePtr)
				{
					s_filePtr = std::make_unique<std::ifstream>(filepath);
					if (s_filePtr->good())
					{
						return true;
					}
					s_filePtr.reset();
				}
				return false;
			}

		protected:
			explicit CommandStreamReader(std::istream& i_streamRef):
				m_streamRef(i_streamRef)
			{
#ifdef _WIN32
				InitializeSRWLock(& m_srwLock);
				InitializeConditionVariable(& m_commandIsReleasedCondition);
#endif
				m_thread = std::thread(&CommandStreamReader::ThreadMain, this);
			}

			~CommandStreamReader()
			{
				// We don't have a real, portable way of interrupting the std::getline and
				// gracefully exiting the thread.  We have to fall back to lower-level
				// mechansims which we know are going to leave the thread sync objects in
				// an unknown state.  Again, this drives us towards a program-lifetime
				// Singleton for this whole thing.
#ifdef _WIN32
				// Windows makes us ruthlessly kill the thread.
				if (TerminateThread(m_thread.native_handle(), 0))
#else
				// Pthreads lets us do a Cancel operation, which defaults to ending the
				// thread when control is in a "cancelation point" function.  Fortunately
				// the thread is going to be blocked in read() or pthread_cond_wait() via
				// std::getline and std::condition_variable for the vast majority of its
				// lifetime, so it cancels basically right away.
				if (pthread_cancel(m_thread.native_handle()) == 0)
#endif
				{
					m_thread.join();
				}
				else
				{
					m_thread.detach();
				}
			}

#ifdef _WIN32
			void ThreadMain()
			{
				AcquireSRWLockExclusive(& m_srwLock);
				while(m_streamRef.good())
				{
					std::getline(m_streamRef, m_command);
					while (m_streamRef and not m_command.empty())
					{
						SleepConditionVariableSRW(& m_commandIsReleasedCondition,
						                          & m_srwLock,
						                          INFINITE,
						                          0);
					}
				}
				ReleaseSRWLockExclusive(& m_srwLock);
			}
#else
			void ThreadMain()
			{
				std::unique_lock lock(m_mutex);
				while(m_streamRef.good())
				{
					std::getline(m_streamRef, m_command);
					while (m_streamRef and not m_command.empty())
					{
						m_commandIsReleasedCondition.wait(lock, [this]() { return m_command.empty(); });
					}
				}
			}
#endif

			static std::unique_ptr<std::ifstream>   s_filePtr;

			// Please note that the member order is intentional for safe construction.
			std::istream &  m_streamRef;
			std::string     m_command;

#ifdef _WIN32
			SRWLOCK                 m_srwLock;
			CONDITION_VARIABLE      m_commandIsReleasedCondition;
#else
			std::mutex              m_mutex;
			std::condition_variable m_commandIsReleasedCondition;
#endif
			std::thread             m_thread;
	};

	std::unique_ptr<std::ifstream> CommandStreamReader::s_filePtr;

}

void M_InitConsoleInputFile (const char* filepath)
{
	CommandStreamReader::OpenInputFile(filepath);
}

std::string M_ConsoleInput (void)
{
	std::string command;
	CommandStreamReader::Get().GetCommand(command);
	return command;
}
