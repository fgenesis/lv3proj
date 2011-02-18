#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include "SDL_func.h"

#include "common.h"
#include "ResourceMgr.h"
#include "AnimParser.h"
#include "PropParser.h"

#include "VFSHelper.h"
#include "VFSFile.h"

// internally referenced files get this postfix to make the file ref map happy
#define FILENAME_TMP_INDIC "|tmp"

ResourceMgr::ResourceMgr()
: _usedMem(0)
{
}

ResourceMgr::~ResourceMgr()
{
    for(FileRefMap::iterator it = _frmap.begin(); it != _frmap.end(); ++it)
    {
        DEBUG(logerror("ResourceMgr: File not unloaded: '%s' ptr "PTRFMT, it->first.c_str(), it->second));
    }
    for(PtrCountMap::iterator it = _ptrmap.begin(); it != _ptrmap.end(); ++it)
    {
        DEBUG(logerror("ResourceMgr: Ptr not unloaded: "PTRFMT" count %u", it->first, it->second));
    }
}

void ResourceMgr::DropUnused(void)
{
    pool.Cleanup();

    bool del;
    do // Anims contain references to SDL_Surfaces, which may be skipped if the Anim is deleted after the
    {  // Surface's count is checked. iterate over tiles again in case we deleted something in the previous loop.
        del = false;
        for(PtrCountMap::iterator it = _ptrmap.begin(); it != _ptrmap.end(); )
        {
            if(!it->second.count)
            {
                _Delete(it->first, it->second);
                _ptrmap.erase(it++);
                del = true;
            }
            else
                ++it;
        }
    } while(del);
}

uint32 ResourceMgr::GetUsedCount(void)
{
    return (uint32)_ptrmap.size();
}

uint32 ResourceMgr::GetUsedMem(void)
{
    //_memMutex.lock();
    register uint32 mem = _usedMem;
    //_memMutex.unlock();
    return mem;
}

void ResourceMgr::_accountMem(uint32 bytes)
{
    //_memMutex.lock();
    _usedMem += bytes;
    //_memMutex.unlock();
}

void ResourceMgr::_unaccountMem(uint32 bytes)
{
    //_memMutex.lock();
    _usedMem -= bytes;
    //_memMutex.unlock();
}

void ResourceMgr::_InitRef(void *ptr, ResourceType rt, void *depdata /* = NULL */)
{
    DEBUG(ASSERT(ptr != NULL));
    if(!ptr)
        return;

    _ptrmap[ptr] = ResStruct(rt, depdata);
}

void ResourceMgr::_IncRef(void *ptr)
{
    DEBUG(ASSERT(ptr != NULL));

    PtrCountMap::iterator it = _ptrmap.find(ptr);
    if(it != _ptrmap.end())
        Falcon::atomicInc(it->second.count);
    else
        logerror("IncRef: "PTRFMT" not reference counted!", ptr);
}

void ResourceMgr::_DecRef(void *ptr, bool del /* = false */)
{
    DEBUG(ASSERT(ptr != NULL));
    PtrCountMap::iterator it = _ptrmap.find(ptr);
    if(it != _ptrmap.end())
    {
        if(!it->second.count) // if we do proper refcounting, we should never try to _DecRef a ptr with refcount 0
        {
            DEBUG(logerror("ResourceMgr::_DecRef("PTRFMT") - already 0 (type %u)", ptr, it->second.count, it->second.rt));
        }
        if( !Falcon::atomicDec(it->second.count) )
        {
            DEBUG(logdebug("ResourceMgr::_DecRef("PTRFMT") - now UNUSED (type %u)", ptr, it->second.count, it->second.rt));
            if(del)
            {
                _Delete(ptr, it->second);
                _ptrmap.erase(it);
            }
            return;
        }
        else
        {
            //DEBUG(logdebug("ResourceMgr::_DecRef("PTRFMT") - now %u (type %u)", ptr, it->second.count, it->second.rt));
        }
    }
    else
    {
        DEBUG(logerror("ResourceMgr::_DecRef("PTRFMT") - NOT FOUND", ptr));
    }
}

void ResourceMgr::_Delete(void *ptr, ResStruct& res)
{
    bool found = false;
    for(FileRefMap::iterator it = _frmap.begin(); it != _frmap.end(); ++it)
    {
        if(it->second == ptr)
        {
            found = true;
            _frmap.erase(it);
            break;
        }
    }
    if(!found)
    {
        DEBUG(logerror("ResourceMgr::_Delete("PTRFMT") - not found in FileRefMap (type %u)", ptr, res.rt));
    }

    switch(res.rt)
    {
        case RESTYPE_MEMBLOCK:
        {
            memblock *mblock = (memblock*)ptr;
            DEBUG(logdebug("ResourceMgr:: Deleting memblock "PTRFMT" with ptr "PTRFMT", size %u",
                ptr, mblock->ptr, mblock->size));
            _unaccountMem(mblock->size);
            delete [] mblock->ptr; // the memblock ptr stores an array!
            delete mblock;
            break;
        }

        case RESTYPE_ANIM:
            DEBUG(logdebug("ResourceMgr:: Deleting Anim "PTRFMT" (%s)", ptr, ((Anim*)ptr)->filename.c_str()));
            delete (Anim*)ptr;
            break;

        case RESTYPE_SDL_SURFACE:
            DEBUG(logdebug("ResourceMgr:: Deleting SDL_Surface "PTRFMT" (%ux%u)", ptr, ((SDL_Surface*)ptr)->w, ((SDL_Surface*)ptr)->h));
            _unaccountMem(SDLfunc_GetSurfaceBytes((SDL_Surface*)ptr));
            SDL_FreeSurface((SDL_Surface*)ptr);
            break;

        case RESTYPE_MIX_CHUNK:
            DEBUG(logdebug("ResourceMgr:: Deleting Mix_Chunk "PTRFMT, ptr));
            _unaccountMem(((Mix_Chunk*)ptr)->alen);
            Mix_FreeChunk((Mix_Chunk*)ptr);
            break;

        case RESTYPE_MIX_MUSIC:
            DEBUG(logdebug("ResourceMgr:: Deleting Mix_Music "PTRFMT, ptr));
            Mix_FreeMusic((Mix_Music*)ptr);
            break;

        default:
            ASSERT(false);
    }

    // if there is another resource the now deleted resource depends on, decref that
    if(res.depdata)
        Drop(res.depdata);
}

SDL_Surface *ResourceMgr::LoadImg(const char *name)
{
    // there may be a recursive call - adding this twice would be not a good idea!
    if(name[0] == '/')
        ++name; // skip first char if already a '/' (can happen for absolute paths in .anim files)
    std::string origfn(name);
    if(origfn.substr(0,4) != "gfx/")
        origfn = "gfx/" + origfn;

    std::string fn,s1,s2,s3,s4,s5;
    SplitFilenameToProps(origfn.c_str(), &fn, &s1, &s2, &s3, &s4, &s5);

    SDL_Surface *img = (SDL_Surface*)_GetPtr(origfn);
    if(img)
    {
        _IncRef((void*)img);
    }
    else
    {
        VFSFile *vf = NULL;

        // we got additional properties
        if(fn != origfn)
        {
            SDL_Surface *origin = LoadImg(fn.c_str());
            if(origin)
            {
                SDL_Rect rect;
                rect.x = atoi(s1.c_str());
                rect.y = atoi(s2.c_str());
                rect.w = atoi(s3.c_str());
                rect.h = atoi(s4.c_str());
                if(!rect.w)
                    rect.w = origin->w - rect.x;
                if(!rect.h)
                    rect.h = origin->h - rect.y;
                bool flipH = s5.find('h') != std::string::npos;
                bool flipV = s5.find('v') != std::string::npos;

                SDL_Surface *section = SDL_CreateRGBSurface(origin->flags, rect.w, rect.h, origin->format->BitsPerPixel,
                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000); // TODO: fix this for big endian
                
                // properly blit alpha values, save original flags + alpha before blitting, and restore after
                uint8 oalpha = origin->format->alpha;
                uint8 oflags = origin->flags;
                SDL_SetAlpha(origin, 0, 0); 
                SDL_BlitSurface(origin, &rect, section, NULL);
                origin->format->alpha = oalpha;
                origin->flags = oflags;

                if(flipH && flipV)
                {
                    img = SurfaceFlipHV(section);
                    //SDL_FreeSurface(section);
                }
                else if(flipH)
                {
                    img = SurfaceFlipH(section);
                    //SDL_FreeSurface(section);
                }
                else if(flipV)
                {
                    img = SurfaceFlipV(section);
                    //SDL_FreeSurface(section);
                }
                else
                {
                    img = section;
                }
                _DecRef(origin);
            }
        }
        else // nothing special, just load image normally
        {
            vf = vfs.GetFile(fn.c_str());
            if(vf && vf->size())
            {
                SDL_RWops *rwop = SDL_RWFromMem((void*)vf->getBuf(), vf->size());
                img = IMG_Load_RW(rwop, 1); // auto-free RWops
                vf->dropBuf(false); // orphan original buf, SDL will take care of the deletion
            }
        }
        if(!img)
        {
            logerror("LoadImg failed: '%s'", origfn.c_str());
            return NULL;
        }
        // convert loaded images into currently used color format.
        // this allows faster blitting because the color formats dont have to be converted
        SDL_Surface *newimg = SDL_DisplayFormatAlpha(img);
        if(newimg && img != newimg)
        {
            SDL_FreeSurface(img);
            img = newimg;
        }

        logdebug("LoadImg: '%s' [%s]" , origfn.c_str(), vf ? vf->getSource() : "*");

        _accountMem(SDLfunc_GetSurfaceBytes(img));
        _SetPtr(origfn, (void*)img);
        _InitRef((void*)img, RESTYPE_SDL_SURFACE);
    }

    return img;
}

Anim *ResourceMgr::LoadAnim(const char *name)
{
    std::string fn("gfx/");
    std::string relpath(_PathStripLast(name));
    std::string loadpath;
    fn += name;

    Anim *ani = (Anim*)_GetPtr(fn);
    if(ani)
    {
        _IncRef((void*)ani);
    }
    else
    {
        // a .anim file is just a text file, so we use the internal text file loader
        memblock *mb = _LoadTextFileInternal((char*)fn.c_str(), true);
        if(!mb)
        {
            logerror("LoadAnim: Failed to open '%s'", fn.c_str());
            return NULL;
        }

        ani = ParseAnimData((char*)mb->ptr, (char*)fn.c_str());
        Drop(mb);

        if(!ani)
        {
            logerror("LoadAnim: Failed to parse '%s'", fn.c_str());
            return NULL;
        }

        // load all additional files referenced in this .anim file
        // pay attention to relative paths in the file, respect the .anim file's directory for this
        for(AnimMap::iterator am = ani->anims.begin(); am != ani->anims.end(); am++)
            for(AnimFrameVector::iterator af = am->second.store.begin(); af != am->second.store.end(); af++)
            {
                loadpath = AddPathIfNecessary(af->filename,relpath);
                af->surface = LoadImg(loadpath.c_str()); // get all images referenced
                if(af->surface)
                    af->callback.ptr(af->surface); // register callback for auto-deletion
                else
                {
                    logerror("LoadAnim: '%s': Failed to open referenced image '%s'", fn.c_str(), loadpath.c_str());
                    // we keep the NULL-ptr anyways
                }
            }

         logdebug("LoadAnim: '%s' [%s]" , fn.c_str(), vfs.GetFile(fn.c_str())->getSource()); // the file must exist

         _SetPtr(fn, (void*)ani);
         _InitRef((void*)ani, RESTYPE_ANIM);
    }

    return ani;
}

Mix_Music *ResourceMgr::LoadMusic(const char *name)
{
    std::string fn("music/");
    fn += name;

    Mix_Music *music = (Mix_Music*)_GetPtr(fn);
    if(music)
    {
        _IncRef((void*)music);
    }
    else
    {
        memblock *mb = _LoadFileInternal(fn.c_str(), true);
        if(mb && mb->size)
        {
            SDL_RWops *rwop = SDL_RWFromConstMem((const void*)mb->ptr, mb->size);
            music = Mix_LoadMUS_RW(rwop);
        }

        if(!music)
        {
            logerror("LoadMusic failed: '%s'", fn.c_str());
            return NULL;
        }
        
        logdebug("LoadMusic: '%s' [%s]" , name, vfs.GetFile(fn.c_str())->getSource());

        _SetPtr(fn, (void*)music);
        _InitRef((void*)music, RESTYPE_MIX_MUSIC, mb); // note that this music depends on mb
    }

    return music;
}

Mix_Chunk *ResourceMgr::LoadSound(const char *name)
{
    std::string fn("sfx/");
    fn += name;

    Mix_Chunk *sound = (Mix_Chunk*)_GetPtr(fn);
    if(sound)
    {
        _IncRef((void*)sound);
    }
    else
    {
        VFSFile *vf = vfs.GetFile(fn.c_str());
        if(vf)
        {
            SDL_RWops *rwop = SDL_RWFromMem((void*)vf->getBuf(), vf->size());
            sound = Mix_LoadWAV_RW(rwop, 1);
            vf->dropBuf(false); // orphan original buf, SDL will take care of the deletion
        }

        if(!sound)
        {
            logerror("LoadSound failed: '%s'", fn.c_str());
            return NULL;
        }

        logdebug("LoadSound: '%s' [%s]" , name, vf->getSource());

        _accountMem(sound->alen);
        _SetPtr(fn, (void*)sound);
        _InitRef((void*)sound, RESTYPE_MIX_CHUNK);
    }

    return sound;
}

memblock *ResourceMgr::LoadFile(const char *name)
{
    std::string t(name); // copying the string is necessary if <name> is hardcoded in the program, and thus really const
    return _LoadFileInternal(t.c_str(), false);
}

memblock *ResourceMgr::_LoadFileInternal(const char *name, bool isTmp)
{
    std::string fn(name);
    if(isTmp)
        fn += FILENAME_TMP_INDIC;
    memblock *mb = (memblock*)_GetPtr(fn);
    if(mb)
    {
        _IncRef((void*)mb);
    }
    else
    {
        VFSFile *vf = vfs.GetFile(name); // this must be the *real* name
        uint32 size;
        if(vf)
        {
            if(!vf->isopen()) // file not open? open, read, and close.
            {
                vf->open();
                size = vf->size();
                if(size)
                {
                    mb = new memblock(new uint8[size], size);
                    vf->read((char*)mb->ptr, size);
                    vf->close();
                }
            }
            else // file already open? save pos, seek, read, restore pos.
            {
                size = vf->size();
                if(size)
                {
                    mb = new memblock(new uint8[size], size);
                    uint64 pos = vf->getpos();
                    vf->seek(0);
                    vf->read((char*)mb->ptr, size);
                    vf->seek(pos);
                }
            }
        }

        if(!mb)
        {
            logerror("LoadFile failed: '%s'", name);
            return NULL;
        }

        logdebug("LoadFile: '%s' [%s], %u bytes" , name, vf->getSource(), size);

        _accountMem(size);
        _SetPtr(fn, (void*)mb);
        _InitRef((void*)mb, RESTYPE_MEMBLOCK);
    }

    return mb;
}

memblock *ResourceMgr::LoadTextFile(const char *name)
{
    std::string t(name); // copying the string is necessary if <name> is hardcoded in the program, and thus really const
    return _LoadTextFileInternal(t.c_str(), false);
}

memblock *ResourceMgr::_LoadTextFileInternal(const char *name, bool isTmp)
{
    std::string fn(name);
    if(isTmp)
        fn += FILENAME_TMP_INDIC;
    memblock *mb = (memblock*)_GetPtr(fn);
    if(mb)
    {
        _IncRef((void*)mb);
    }
    else
    {
        VFSFile *vf = vfs.GetFile(name);
        if(vf)
        {
            if(vf->isopen())
                vf->close();
            uint32 size = vf->size();

            if(size && vf->open(NULL, "r"))
            {
                mb = new memblock(new uint8[size + 4], size); // extra padding
                uint8 *writeptr = mb->ptr;
                uint32 realsize = 0;
                while(!vf->iseof())
                {
                    int bytes = vf->read((char*)writeptr, 0x800);
                    writeptr += bytes;
                    realsize += bytes;
                }
                memset(writeptr, 0, size - realsize + 4); // zero out remaining space
                mb->size = realsize;
                vf->close();
            }
        }

        if(!mb)
        {
            logerror("LoadTextFile failed: '%s'", name);
            return NULL;
        }

        logdebug("LoadTextFile: '%s' [%s]" , name, vf->getSource());

        _accountMem(mb->size);
        _SetPtr(fn, (void*)mb);
        _InitRef((void*)mb, RESTYPE_MEMBLOCK);
    }

    return mb;
}

std::string ResourceMgr::GetPropForFile(const char *fn, const char *prop)
{
    PropMap::iterator pi = _fprops.find(fn);
    if(pi != _fprops.end())
    {
        std::map<std::string,std::string>::iterator it = pi->second.find(prop);
        if(it != pi->second.end())
            return it->second;
    }
    
    return std::string();
}

void ResourceMgr::SetPropForFile(const char *fn, const char *prop, const char *what)
{
    std::map<std::string,std::string>& fprop = _fprops[fn];
    std::map<std::string,std::string>::iterator it = fprop.find(prop);
    if(it != fprop.end())
        fprop.erase(it);
    fprop[prop] = what;
}




// extern, global (since we aren't using singletons here)
ResourceMgr resMgr;
