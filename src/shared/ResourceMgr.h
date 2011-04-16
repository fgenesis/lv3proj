#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <falcon/mt.h>
#include "UndefUselessCrap.h"

#include "VFSHelper.h"
#include "DelayedDeletable.h"

struct Anim;


typedef std::map<std::string, std::map<std::string, std::string> > PropMap;

class ResourceMgr
{
    enum ResourceType
    {
        RESTYPE_MEMBLOCK,
        RESTYPE_ANIM,
        RESTYPE_SDL_SURFACE,
        RESTYPE_MIX_CHUNK,
        RESTYPE_MIX_MUSIC,
    };

    struct ResStruct
    {
        ResStruct() : rt(RESTYPE_MEMBLOCK), count(1), depdata(NULL) {}
        ResStruct(ResourceType r, void *dep = NULL, SDL_RWops *rw = NULL) : rt(r), count(1), depdata(dep), rwop(rw) {}
        volatile Falcon::int32 count;
        ResourceType rt;
        void *depdata;
        SDL_RWops *rwop;
    };

    typedef std::map<void*, ResStruct> PtrCountMap;
    typedef std::map<std::string, void*> FileRefMap;

public:
    ResourceMgr();
    ~ResourceMgr();
    void DbgCheckEmpty(void);

    template <class T> inline void Drop(T *ptr, bool del = false) { _DecRef((void*)ptr, del); }
    void DropUnused(void);

    SDL_Surface *LoadImg(const char *name);
    Anim *LoadAnim(const char *name);
    Mix_Music *LoadMusic(const char *name);
    Mix_Chunk *LoadSound(const char *name);
    memblock *LoadFile(const char *name);
    memblock *LoadTextFile(const char *name);
    void SetPropForFile(const char *fn, const char *prop, const char *what);
    std::string GetPropForFile(const char *fn, const char *prop);
    std::string GetPropForMusic(const char *fn, const char *prop) { return GetPropForFile((std::string("music/") + fn).c_str(), prop); }
    uint32 GetUsedCount(void); // amount of resources
    uint32 GetUsedMem(void); // estimated total resource memory consumption

    VFSHelper vfs;
    DeletablePool pool;

private:
    memblock *_LoadFileInternal(const char *name, bool isTmp);
    memblock *_LoadTextFileInternal(const char *name, bool isTmp);

    inline void *_GetPtr(const std::string& fn)
    {
        FileRefMap::iterator it = _frmap.find(fn);
        return (it == _frmap.end() ? NULL : it->second);
    }
    inline void _SetPtr(const std::string& fn, void *ptr)
    {
        _frmap[fn] = ptr;
    }
    void _InitRef(void *ptr, ResourceType rt, void *depdata = NULL, SDL_RWops *rwop = NULL);
    void _IncRef(void *ptr);
    void _DecRef(void *ptr, bool del = false);
    void _Delete(void *ptr, ResStruct& rt);
    PtrCountMap _ptrmap;
    FileRefMap _frmap;
    PropMap _fprops;

    void _accountMem(uint32 bytes);
    void _unaccountMem(uint32 bytes);
    uint32 _usedMem;
    //Mutex _memMutex; // this is not (yet) needed; unlike ref-counting, memory allocation/deallocation is done in a single thread
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


#endif
