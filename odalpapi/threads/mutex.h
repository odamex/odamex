#ifndef _MUTEX_H
#define _MUTEX_H

namespace odalpapi {
namespace threads {

class Mutex
{
public:
	virtual ~Mutex() {}

	virtual int getLock() = 0;

	virtual int tryLock() = 0;

	virtual int unlock() = 0;
};

}} // namespace

#endif
