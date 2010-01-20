#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include <map>

enum MemFreeType
{
    ALLOC_FREESURFACE, // for SDL surfaces
    ALLOC_FREE, // for SDL stuff
    ALLOC_DELETE // for custom stuff
};

struct ResStruct
{
    ResStruct(void *p, MemFreeType f) : ptr(p), fty(f), refcount(1) {}
    ResStruct() {}
    void *ptr;
    uint32 refcount;
    MemFreeType fty;
    inline void incr(void) { ++refcount; }
    void drop(void);
};

typedef std::map<std::string, ResStruct> ResourceStore;


// TODO: replace this with some refcounting sooner or later

class ResourceMgr
{
public:
    ~ResourceMgr();
    bool AddResource(char *name, void *res, MemFreeType fty);
    void *GetResource(char *name, bool count = false);
    void DropResource(char *name);


private:
    ResourceStore _store;
    
};


extern ResourceMgr resMgr;

#endif
