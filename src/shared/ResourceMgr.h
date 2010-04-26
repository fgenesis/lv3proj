#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

struct Anim;


typedef std::map<std::string, std::map<std::string, std::string> > PropMap;

class ResourceMgr
{
    enum ResourceType
    {
        RESTYPE_MEMBLOCK,
        RESTYPE_ANIM,
        RESTYPE_SDL_SURFACE,
        RESTYPE_MIX_CHUNK
    };

    struct ResStruct
    {
        ResStruct() : rt(RESTYPE_MEMBLOCK), count(1) {}
        ResStruct(ResourceType r) : rt(r), count(1) {}
        uint32 count;
        ResourceType rt;
    };

    typedef std::map<void*, ResStruct> PtrCountMap;
    typedef std::map<std::string, void*> FileRefMap;

public:
    ~ResourceMgr();

    template <class T> inline void Drop(T *ptr) { _DecRef((void*)ptr); }
    void DropUnused(void);

    SDL_Surface *LoadImg(char *name);
    Anim *LoadAnim(char *name);
    Mix_Music *LoadMusic(char *name);
    memblock *LoadFile(char *name);
    memblock *LoadTextFile(char *name);
    void SetPropForFile(char *fn, char *prop, char *what);
    std::string GetPropForFile(char *fn, char *prop);
    std::string GetPropForMusic(char *fn, char *prop) { return GetPropForFile((char*)(std::string("music/") + fn).c_str(), prop); }
    void LoadPropsInDir(char *);

private:
    inline void *_GetPtr(std::string& fn)
    {
        FileRefMap::iterator it = _frmap.find(fn);
        return (it == _frmap.end() ? NULL : it->second);
    }
    inline void _SetPtr(std::string& fn, void *ptr)
    {
        _frmap[fn] = ptr;
    }
    void _IncRef(void *ptr, ResourceType rt);
    void _DecRef(void *ptr);
    void _Delete(void *ptr, ResourceType rt);
    PtrCountMap _ptrmap;
    FileRefMap _frmap;
    PropMap _fprops;
};


extern ResourceMgr resMgr;

// ResourceCallback: used for objects that are stored within the ResourceMgr
// _ptr must point to the resource ptr stored in the mgr.
template <class T> class ResourceCallback
{
private:
    T *_ptr;

public:
    ResourceCallback(T *p): _ptr(p) {}
    ResourceCallback(): _ptr(NULL) {}
    ~ResourceCallback()
    {
        if(_ptr)
            resMgr.Drop(_ptr);
    }
    inline void ptr(T *p) { _ptr = p; }
};

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
            DEBUG(_printDebug());
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
