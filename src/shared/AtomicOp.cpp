#include "AtomicOp.h"


#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <SDL/SDL.h>
static SDL_mutex *mutex = NULL;
static Atomic dummy; // for auto-initialization
#endif


Atomic::Atomic()
{
#ifndef _WIN32
    if(!mutex)
        mutex = SDL_CreateMutex();
#endif
}

Atomic::~Atomic()
{
#ifndef _WIN32
    if(mutex)
        SDL_DestroyMutex(mutex);
#endif
}


int Atomic::Incr(volatile int &i)
{
#ifdef _WIN32
    volatile LONG* dp = (volatile LONG*) &i;
    return InterlockedIncrement( dp );
#else
    SDL_LockMutex(mutex);
    register int r = ++i;
    SDL_UnlockMutex(mutex);

    return r;
#endif
}

int Atomic::Decr(volatile int &i)
{
#ifdef _WIN32
    volatile LONG* dp = (volatile LONG*) &i;
    return InterlockedDecrement( dp );
#else
    SDL_LockMutex(mutex);
    register int r = --i;
    SDL_UnlockMutex(mutex);

    return r;
#endif
}