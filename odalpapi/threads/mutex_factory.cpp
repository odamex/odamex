#include "mutex_factory.h"

namespace odalpapi {
namespace threads {

MutexFactory::MutexFactory() :
	m_factory(NULL)
{
}

MutexFactory& MutexFactory::inst()
{
	static MutexFactory mufact;
	return mufact;
}

Mutex* MutexFactory::createMutex() const
{
	if(NULL != m_factory)
	{
		return m_factory->createMutex();
	}

	return NULL;
}

void MutexFactory::setFactory(AbstractMutexFactory* factory)
{
	if(NULL != m_factory)
	{
		delete m_factory;
	}

	m_factory = factory;
}

}} // namespace
