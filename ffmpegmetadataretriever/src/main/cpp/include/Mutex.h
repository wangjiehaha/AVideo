//
// Created by user on 2020/3/12.
//

#ifndef AVIDEO_MUTEX_H
#define AVIDEO_MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <pthread.h>

#include "Error.h"

class Condition;

/*
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();

    Mutex(const char *name);

    Mutex(int type, const char *name = NULL);

    ~Mutex();

    // lock or unlock the mutex
    status_t lock();

    void unlock();

    // lock if possible; returns 0 on success, error otherwise
    status_t tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(Mutex &mutex) : mLock(mutex) { mLock.lock(); }

        inline Autolock(Mutex *mutex) : mLock(*mutex) { mLock.lock(); }

        inline ~Autolock() { mLock.unlock(); }

    private:
        Mutex &mLock;
    };

private:
    friend class Condition;

    Mutex(const Mutex &);

    Mutex &operator=(const Mutex &);

    pthread_mutex_t mMutex;

};

inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::Mutex(const char *name) {
    pthread_mutex_init(&mMutex, NULL);
}

inline Mutex::Mutex(int type, const char *name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}

inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}

inline status_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}

inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}

inline status_t Mutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}

typedef Mutex::Autolock AutoMutex;

#endif //AVIDEO_MUTEX_H
