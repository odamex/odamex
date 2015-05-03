#ifndef _AGAR_MUTEX_H
#define _AGAR_MUTEX_H

#include <agar/core.h>

namespace agOdalaunch {

class AgarMutex : public odalpapi::threads::Mutex
{
public:
	AgarMutex() { AG_MutexInit(&m_Mutex); }

	int getLock() { AG_MutexLock(&m_Mutex); }

	int tryLock() { return AG_MutexTryLock(&m_Mutex); }

	int unlock() { AG_MutexUnlock(&m_Mutex); }

private:
	AG_Mutex m_Mutex;
};

} // namespace

#endif
