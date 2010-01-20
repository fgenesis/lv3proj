#include <SDL/SDL.h>

#include "common.h"
#include "ResourceMgr.h"

void ResStruct::drop(void)
{
    if(! --refcount)
    {
        if(fty == ALLOC_DELETE)
            delete ptr;
        else if(fty == ALLOC_FREESURFACE)
            SDL_FreeSurface((SDL_Surface*)ptr);
        else if(fty == ALLOC_FREE)
            free(ptr);
        else
            ASSERT(false); // whoops
    }
}

ResourceMgr::~ResourceMgr()
{
    for(ResourceStore::iterator it = _store.begin(); it != _store.end(); it++)
    {
        it->second.refcount = 1; // enforce deletion
        it->second.drop();
    }
}


bool ResourceMgr::AddResource(char *name, void *res, MemFreeType fty)
{
    ResourceStore::iterator it = _store.find(name);
    if(it != _store.end())
    {
        // already exists, increase refcount
        it->second.incr();
        return false;
    }

    _store[name] = ResStruct(res,fty);
    return true;
}

void *ResourceMgr::GetResource(char *name, bool count /* = false*/)
{
    ResourceStore::iterator it = _store.find(name);
    if(it != _store.end())
    {
        if(count)
            it->second.incr();
        return it->second.ptr;
    }

    return NULL;
}

void ResourceMgr::DropResource(char *name)
{
    ResourceStore::iterator it = _store.find(name);
    if(it != _store.end())
    {
        it->second.drop();
    }
}

// extern, global (since we arent using singletons here)
ResourceMgr resMgr;
