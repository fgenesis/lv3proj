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
        RESTYPE_GENERIC,
        RESTYPE_ANIM,
        RESTYPE_SDL_SURFACE,
        RESTYPE_MIX_CHUNK
    };

    struct ResStruct
    {
        ResStruct() : rt(RESTYPE_GENERIC), count(1) {}
        ResStruct(ResourceType r) : rt(r), count(1) {}
        uint32 count;
        ResourceType rt;
    };

    typedef std::map<void*, ResStruct> PtrCountMap;
    typedef std::map<std::string, void*> FileRefMap;

public:
    ~ResourceMgr();

    template <class T> inline bool Drop(T *ptr) { _DecRef((void*)ptr); }

    SDL_Surface *LoadImg(char *name, bool count = false);
    Anim *LoadAnim(char *name, bool count = false);
    Mix_Music *LoadMusic(char *name, bool count = false);
    memblock *LoadFile(char *name, char *mode = "r", bool count = false);
    char *LoadTextFile(char *name, char *mode = "r", bool count = false);
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

#endif
