#ifndef _AGAR_MUTEX_H
#define _AGAR_MUTEX_H

#include <agar/core.h>

#if !AG_VERSION_ATLEAST(1,4,2)
    #define AG_MutexTryLock AG_MutexTrylock
#endif

namespace agOdalaunch {

class AgarMutex : public odalpapi::threads::Mutex
{
public:
	AgarMutex() { AG_MutexInit(&m_Mutex); }

	int getLock() { AG_MutexLock(&m_Mutex); return 0; }

	int tryLock() { return AG_MutexTryLock(&m_Mutex); }

	int unlock() { AG_MutexUnlock(&m_Mutex); return 0; }

private:
	AG_Mutex m_Mutex;
};

} // namespace

#endif
