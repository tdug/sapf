#pragma once

#ifdef SAPF_APPLE_LOCK
#include <os/lock.h>

typedef os_unfair_lock Lock;

#define LOCK_DECLARE(name) mutable Lock name = OS_UNFAIR_LOCK_INIT

class SpinLocker
{
    Lock& lock;
public:
    SpinLocker(Lock& inLock) : lock(inLock)
    {
        os_unfair_lock_lock(&lock);
    }
    ~SpinLocker()
    {
        os_unfair_lock_unlock(&lock);
    }
};

#else
#include <mutex>
#include <shared_mutex>
#include <thread>

typedef std::shared_mutex Lock;

#define LOCK_DECLARE(name) mutable Lock name

class SpinLocker
{
    std::unique_lock<Lock> w_lock;
public:
    SpinLocker(Lock& inLock) : w_lock(inLock)
    {}
    ~SpinLocker()
    {}
};

#endif // SAPF_APPLE_LOCK
