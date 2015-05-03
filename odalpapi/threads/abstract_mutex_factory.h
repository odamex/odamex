#ifndef _ABSTRACT_MUTEX_FACTORY_H
#define _ABSTRACT_MUTEX_FACTORY_H

#include <cstddef>
#include "mutex.h"

namespace odalpapi {
namespace threads {

class AbstractMutexFactory
{
public:
	virtual ~AbstractMutexFactory() {}

	virtual Mutex* createMutex() const { return NULL; }
};

}} // namespace

#endif
