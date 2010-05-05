#ifndef SELFREFCOUNTER_H
#define SELFREFCOUNTER_H

// SelfRefCounter: used for any type of objects that are NOT stored in the ResourceMgr.
// self must point to the object that holds the counter.
template <class T> class SelfRefCounter
{
private:
    T *self;
    uint32 c;
    SelfRefCounter(SelfRefCounter& r); // forbid copy constructor
    inline uint32 _deref(void)
    {
        --c;
        uint32 cc = c; // copy c, in case we get deleted
        if(!c)
        {
            //DEBUG(_printDebug());
            delete self;
        }
        return cc;
    }

    void _printDebug(void);

public:
    SelfRefCounter(T *p): self(p), c(1) {}
    ~SelfRefCounter() { DEBUG(ASSERT(c == 0)); }
    inline uint32 count(void) { return c; }

    // post-increment
    inline uint32 operator++(int) { ++c; return c; }
    inline uint32 operator--(int) { return _deref(); }

    // pre-increment
    inline uint32 operator++(void) { ++c; return c; }
    inline uint32 operator--(void) { return _deref(); }
};

#ifdef _DEBUG

/*template <> void SelfRefCounter<BasicTile>::_printDebug(void)
{
BasicTile *tile = (BasicTile*)self;
DEBUG(logdebug("Refcount: drop "PTRFMT" (%s)", tile, tile->GetFilename()));
}*/

template <class T> void SelfRefCounter<T>::_printDebug(void)
{
    DEBUG(logdebug("Refcount: drop "PTRFMT, self));
}


#endif // _DEBUG

#endif
