#ifndef _MUTEX_FACTORY_H
#define _MUTEX_FACTORY_H

#include "mutex.h"
#include "abstract_mutex_factory.h"

namespace odalpapi {
namespace threads {

class MutexFactory
{
public:
	static MutexFactory& inst();

	Mutex* createMutex() const;

	void setFactory(AbstractMutexFactory* factory);

private:
	MutexFactory();
	MutexFactory(const MutexFactory&);
	MutexFactory& operator=(const MutexFactory&);

	AbstractMutexFactory* m_factory;
};

}} // namespace

#endif
