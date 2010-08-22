#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "VFSHelper.h"

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
        ResStruct() : rt(RESTYPE_MEMBLOCK), count(1) {}
        ResStruct(ResourceType r) : rt(r), count(1) {}
        uint32 count;
        ResourceType rt;
    };

    typedef std::map<void*, ResStruct> PtrCountMap;
    typedef std::map<std::string, void*> FileRefMap;

public:
    ~ResourceMgr();

    template <class T> inline void Drop(T *ptr, bool del = false) { _DecRef((void*)ptr, del); }
    void DropUnused(void);

    SDL_Surface *LoadImg(char *name);
    Anim *LoadAnim(char *name);
    Mix_Music *LoadMusic(char *name);
    Mix_Chunk *LoadSound(char *name);
    memblock *LoadFile(char *name);
    memblock *LoadTextFile(char *name);
    void SetPropForFile(char *fn, char *prop, char *what);
    std::string GetPropForFile(char *fn, char *prop);
    std::string GetPropForMusic(char *fn, char *prop) { return GetPropForFile((char*)(std::string("music/") + fn).c_str(), prop); }

    VFSHelper vfs;

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
    void _DecRef(void *ptr, bool del = false);
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


#endif
