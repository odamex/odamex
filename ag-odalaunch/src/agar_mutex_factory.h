#ifndef _AGAR_MUTEX_FACTORY_H
#define _AGAR_MUTEX_FACTORY_H

#include "threads/abstract_mutex_factory.h"
#include "agar_mutex.h"

namespace agOdalaunch {

class AgarMutexFactory : public odalpapi::threads::AbstractMutexFactory
{
public:
	odalpapi::threads::Mutex* createMutex() const { return new AgarMutex; }
};

} // namespace

#endif
