// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	 AG_Thread C++ Wrapper
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _ODA_THREAD_H
#define _ODA_THREAD_H

#ifndef CALL_MEMBER_FN
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#endif

/**
 * agOdalaunch namespace.
 *
 * All code for the ag-odalaunch launcher is contained within the agOdalaunch
 * namespace.
 */
namespace agOdalaunch {

const size_t NUM_THREADS = 5;

class ODA_ThreadBase
{
public:
	virtual ~ODA_ThreadBase() {}
};

typedef void *(ODA_ThreadBase::*THREAD_FUNC_PTR)(void *);

typedef struct 
{
	ODA_ThreadBase  *classPtr;
	THREAD_FUNC_PTR  funcPtr;
	AG_Mutex        *rMutex;
	bool            *running;
	void            *arg;
} ThreadArg_t;

class ODA_Thread
{
public:
	ODA_Thread() { m_Running = false; m_RMutex = NULL; }

	int Create(ODA_ThreadBase *classPtr, THREAD_FUNC_PTR funcPtr, void *arg)
	{
		if(m_RMutex)
		{
			if(IsRunning())
				return -1;
			else
				AG_ThreadJoin(m_Thread, NULL);
		}
		else
		{
			m_RMutex = new AG_Mutex;
			AG_MutexInit(m_RMutex);
		}

		ThreadArg_t *targ = new(ThreadArg_t);

		m_Running = true;

		targ->classPtr = classPtr;
		targ->funcPtr = funcPtr;
		targ->rMutex = m_RMutex;
		targ->running = &m_Running;
		targ->arg = arg;

		AG_ThreadCreate(&m_Thread, CallThreadFunc, targ);

		return 0;
	}

	static void *CallThreadFunc(void *arg)
	{
		ThreadArg_t *targ = (ThreadArg_t *)arg;

		if(targ && targ->classPtr && targ->rMutex && targ->running)
		{
			void *ret;

			ret = CALL_MEMBER_FN(*targ->classPtr, targ->funcPtr)(targ->arg);

			AG_MutexLock(targ->rMutex);
			*targ->running = false;
			AG_MutexUnlock(targ->rMutex);

			delete targ;

			AG_ThreadExit(ret);
		}

		AG_ThreadExit(NULL);

		return NULL;
	}

	bool IsRunning()
	{
		bool val;

		if(!m_RMutex)
			return false;

		AG_MutexLock(m_RMutex);
		val = m_Running;
		AG_MutexUnlock(m_RMutex);

		return val;
	}

	void Cancel() { AG_ThreadCancel(m_Thread); }
	void Join(void **exitVal) { AG_ThreadJoin(m_Thread, exitVal); }
	void Kill(int signal) { AG_ThreadKill(m_Thread, signal); }

private:
	AG_Thread  m_Thread;
	AG_Mutex  *m_RMutex;
	bool       m_Running;
};

} // namespace

#endif
