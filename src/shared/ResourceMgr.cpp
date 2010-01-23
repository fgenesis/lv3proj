#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "common.h"
#include "ResourceMgr.h"
#include "AnimParser.h"

ResourceMgr::~ResourceMgr()
{
    for(FileRefMap::iterator it = _frmap.begin(); it != _frmap.end(); it++)
    {
        DEBUG(logerror("ResourceMgr: File not unloaded: '%s' ptr "PTRFMT, it->first.c_str(), it->second));
    }
    for(PtrCountMap::iterator it = _ptrmap.begin(); it != _ptrmap.end(); it++)
    {
        DEBUG(logerror("ResourceMgr: Ptr not unloaded: "PTRFMT" count %u", it->first, it->second));
    }
}

void ResourceMgr::_IncRef(void *ptr, ResourceType rt)
{
    if(!ptr)
        return;

    PtrCountMap::iterator it = _ptrmap.find(ptr);
    if(it != _ptrmap.end())
        ++(it->second.count);
    else
        _ptrmap[ptr] = ResStruct(rt);
}

void ResourceMgr::_DecRef(void *ptr)
{
    PtrCountMap::iterator it = _ptrmap.find(ptr);
    if(it != _ptrmap.end())
        if( !(--(it->second.count)) )
            _Delete(it->first, it->second.rt);
}

void ResourceMgr::_Delete(void *ptr, ResourceType rt)
{
    for(FileRefMap::iterator it = _frmap.begin(); it != _frmap.end(); it++)
    {
        if(it->second == ptr)
        {
            _frmap.erase(it);
            break;
        }
    }

    switch(rt)
    {
        case RESTYPE_GENERIC:
            delete ((memblock*)ptr)->ptr;
            delete (memblock*)ptr;
            break;

        case RESTYPE_ANIM:
            delete (Anim*)ptr;
            break;

        case RESTYPE_SDL_SURFACE:
            SDL_FreeSurface((SDL_Surface*)ptr);
            break;

        default:
            ASSERT(false);
    }
}

SDL_Surface *ResourceMgr::LoadImage(char *name, bool count /* = false*/)
{
    std::string fn("gfx/");
    fn += name;

    bool loaded = false;
    SDL_Surface *img = (SDL_Surface*)_GetPtr(fn);
    if(!img)
    {
        img = IMG_Load(fn.c_str());
        loaded = true;
    }
    if(count || loaded)
    {
        _SetPtr(fn, (void*)img);
        _IncRef((void*)img, RESTYPE_SDL_SURFACE);
    }
    return img;
}

Anim *ResourceMgr::LoadAnim(char *name, bool count /* = false */)
{
    std::string fn("gfx/");
    fn += name;

    bool loaded = false;
    Anim *ani = (Anim*)_GetPtr(fn);
    if(!ani)
    {
        ani = LoadAnimFile((char*)fn.c_str());
        loaded = true;
    }
    if(count || loaded)
    {
        _SetPtr(fn, (void*)ani);
        _IncRef((void*)ani, RESTYPE_ANIM);
    }
    return ani;
}

memblock *ResourceMgr::LoadFile(char *name, bool count /* = false */)
{
    std::string fn(name);
    memblock *mb = (memblock*)_GetPtr(fn);
    bool loaded = false;
    if(!mb)
    {
        FILE *fh = fopen(name, "r");
        if(!fh)
            return NULL;

        fseek(fh, 0, SEEK_END);
        uint32 size = ftell(fh);
        rewind(fh);

        if(!size)
            return NULL;

        mb = new memblock(new uint8[size], size);
        fread(mb->ptr, 1, size, fh);
        fclose(fh);
        loaded = true;
    }
    if(count || loaded)
    {
        _SetPtr(fn, (void*)mb);
        _IncRef((void*)mb, RESTYPE_GENERIC);
    }

    return mb;
}


// extern, global (since we arent using singletons here)
ResourceMgr resMgr;
