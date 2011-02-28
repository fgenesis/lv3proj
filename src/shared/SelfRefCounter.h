#ifndef SELFREFCOUNTER_H
#define SELFREFCOUNTER_H

// use low level locking functions from falcon
#include <falcon/mt.h>

// because falcon includes windows.h
#include "UndefUselessCrap.h"

// SelfRefCounter: used for any type of objects that are NOT stored in the ResourceMgr.
// self must point to the object that holds the counter.
template <class T, bool DELSELF = true> class SelfRefCounter
{
private:
    T *self;
    volatile Falcon::int32 c;
    SelfRefCounter(SelfRefCounter& r); // forbid copy constructor
    inline uint32 _deref(void)
    {
        uint32 cc = (uint32)Falcon::atomicDec(c); // copy c, in case we get deleted
        if(DELSELF && !cc)
        {
            delete self;
        }

        return cc;
    }

public:
    SelfRefCounter(T *p): self(p), c(1) {}
    ~SelfRefCounter() { DEBUG(ASSERT(c <= 1)); } // its ok if the last reference calls delete instead of _deref()
    inline uint32 count(void) { return c; }

    // post-increment
    inline uint32 operator++(int) { return (uint32)Falcon::atomicInc(c); }
    inline uint32 operator--(int) { return _deref(); }

    // pre-increment
    inline uint32 operator++(void) { uint32 cc = c; Falcon::atomicInc(c); return cc; }
    inline uint32 operator--(void) { uint32 cc = c; _deref(); return cc; }
};


#endif
