#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include "SDL_func.h"

#include "common.h"
#include "ResourceMgr.h"
#include "AnimParser.h"
#include "PropParser.h"

#include "LVPAFile.h"

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
    _vfs.FreeAll();
}

void ResourceMgr::DropUnused(void)
{
    bool del;
    do // Anims contain references to SDL_Surfaces, which may be skipped if the Anim is deleted after the
    {  // Surface's count is checked. iterate over tiles again in case we deleted something in the previous loop.
        del = false;
        for(PtrCountMap::iterator it = _ptrmap.begin(); it != _ptrmap.end(); )
        {
            if(!it->second.count)
            {
                _Delete(it->first, it->second.rt);
                _ptrmap.erase(it++);
                del = true;
            }
            else
                it++;
        }
    } while(del);
}

void ResourceMgr::_IncRef(void *ptr, ResourceType rt)
{
    DEBUG(ASSERT(ptr != NULL));
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
    DEBUG(ASSERT(ptr != NULL));
    PtrCountMap::iterator it = _ptrmap.find(ptr);
    if(it != _ptrmap.end())
    {
        if(!it->second.count) // if we do proper refcounting, we should never try to _DecRef a ptr with refcount 0
        {
            DEBUG(logerror("ResourceMgr::_DecRef("PTRFMT") - already 0 (type %u)", ptr, it->second.count, it->second.rt));
        }
        if( !(--(it->second.count)) )
        {
            DEBUG(logdebug("ResourceMgr::_DecRef("PTRFMT") - now UNUSED (type %u)", ptr, it->second.count, it->second.rt));
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

void ResourceMgr::_Delete(void *ptr, ResourceType rt)
{
    bool found = false;
    for(FileRefMap::iterator it = _frmap.begin(); it != _frmap.end(); it++)
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
        DEBUG(logerror("ResourceMgr::_Delete("PTRFMT") - not found in FileRefMap (type %u)", ptr, rt));
    }

    switch(rt)
    {
        case RESTYPE_MEMBLOCK:
            DEBUG(logdebug("ResourceMgr:: Deleting memblock "PTRFMT" with ptr "PTRFMT", size %u",
                ptr, ((memblock*)ptr)->ptr, ((memblock*)ptr)->size));
            delete ((memblock*)ptr)->ptr;
            delete (memblock*)ptr;
            break;

        case RESTYPE_ANIM:
            DEBUG(logdebug("ResourceMgr:: Deleting Anim "PTRFMT" (%s)", ptr, ((Anim*)ptr)->filename.c_str()));
            delete (Anim*)ptr;
            break;

        case RESTYPE_SDL_SURFACE:
            DEBUG(logdebug("ResourceMgr:: Deleting SDL_Surface "PTRFMT" (%ux%u)", ptr, ((SDL_Surface*)ptr)->w, ((SDL_Surface*)ptr)->h));
            SDL_FreeSurface((SDL_Surface*)ptr);
            break;

        case RESTYPE_MIX_CHUNK:
            DEBUG(logdebug("ResourceMgr:: Deleting Mix_Chunk "PTRFMT, ptr));
            Mix_FreeChunk((Mix_Chunk*)ptr);
            break;

        case RESTYPE_MIX_MUSIC:
            DEBUG(logdebug("ResourceMgr:: Deleting Mix_Music "PTRFMT, ptr));
            Mix_FreeMusic((Mix_Music*)ptr);
            break;

        default:
            ASSERT(false);
    }
}

SDL_Surface *ResourceMgr::LoadImg(char *name)
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
    if(!img)
    {
        // we got additional properties
        if(fn != origfn)
        {
            SDL_Surface *origin = LoadImg((char*)fn.c_str());
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
                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
                
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
            LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
            if(vfile.mb.ptr)
            {
                SDL_RWops *rwop = SDL_RWFromMem(vfile.mb.ptr, vfile.mb.size);
                img = IMG_Load_RW(rwop, 1);
                vfile.src->Free(fn.c_str());
            }
            else
            {   // if the file was not found in the virtual file system, try loading from disk
                img = IMG_Load(fn.c_str());
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
        logdebug("LoadImg: '%s' -> "PTRFMT, origfn.c_str(), img);
    }

    _SetPtr(origfn, (void*)img);
    _IncRef((void*)img, RESTYPE_SDL_SURFACE);

    return img;
}

Anim *ResourceMgr::LoadAnim(char *name)
{
    std::string fn("gfx/");
    std::string relpath(_PathStripLast(name));
    std::string loadpath;
    fn += name;

    Anim *ani = (Anim*)_GetPtr(fn);
    if(!ani)
    {
        LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
        if(vfile.mb.ptr)
        {
            ani = ParseAnimData((char*)vfile.mb.ptr, (char*)fn.c_str());
            vfile.src->Free(fn.c_str());
        }
        else
        {   // if the file was not found in the virtual file system, try loading from disk
            ani = LoadAnimFile((char*)fn.c_str());
        }
        if(ani)
        {
            // load all additional files referenced in this .anim file
            // pay attention to relative paths in the file, respect the .anim file's directory for this
            for(AnimMap::iterator am = ani->anims.begin(); am != ani->anims.end(); am++)
                for(AnimFrameVector::iterator af = am->second.store.begin(); af != am->second.store.end(); af++)
                {
                    loadpath = AddPathIfNecessary(af->filename,relpath);
                    af->surface = resMgr.LoadImg((char*)loadpath.c_str()); // get all images referenced
                    af->callback.ptr(af->surface); // register callback for auto-deletion
                }
        }
        else
        {
            logerror("LoadAnim failed: '%s'", fn.c_str());
            return NULL;
        }
        logdebug("LoadAnim: '%s' -> "PTRFMT, fn.c_str(), ani);
    }

    _SetPtr(fn, (void*)ani);
    _IncRef((void*)ani, RESTYPE_ANIM);

    return ani;
}

Mix_Music *ResourceMgr::LoadMusic(char *name)
{
    std::string fn("music/");
    fn += name;

    Mix_Music *music = (Mix_Music*)_GetPtr(fn);
    if(!music)
    {
        LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
        if(vfile.mb.ptr)
        {
            SDL_RWops *rwop = SDL_RWFromMem(vfile.mb.ptr, vfile.mb.size);
            music = Mix_LoadMUS_RW(rwop);
            SDL_FreeRW(rwop);
            vfile.src->Free(fn.c_str());
        }
        else
        {   // if the file was not found in the virtual file system, try loading from disk
            music = Mix_LoadMUS((char*)fn.c_str());
        }
        if(!music)
        {
            logerror("LoadMusic failed: '%s'", fn.c_str());
            return NULL;
        }
        logdebug("LoadMusic: '%s' -> "PTRFMT, fn.c_str(), music);
    }

    _SetPtr(fn, (void*)music);
    _IncRef((void*)music, RESTYPE_MIX_MUSIC);

    return music;
}

Mix_Chunk *ResourceMgr::LoadSound(char *name)
{
    std::string fn("sfx/");
    fn += name;

    Mix_Chunk *sound = (Mix_Chunk*)_GetPtr(fn);
    if(!sound)
    {
        LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
        if(vfile.mb.ptr)
        {
            SDL_RWops *rwop = SDL_RWFromMem(vfile.mb.ptr, vfile.mb.size);
            sound = Mix_LoadWAV_RW(rwop, 1);
            vfile.src->Free(fn.c_str());
        }
        else
        {   // if the file was not found in the virtual file system, try loading from disk
            sound = Mix_LoadWAV((char*)fn.c_str());
        }
        if(!sound)
        {
            logerror("LoadSound failed: '%s'", fn.c_str());
            return NULL;
        }
        logdebug("LoadSound: '%s' -> "PTRFMT, fn.c_str(), sound);
    }

    _SetPtr(fn, (void*)sound);
    _IncRef((void*)sound, RESTYPE_MIX_CHUNK);

    return sound;
}


memblock *ResourceMgr::LoadFile(char *name)
{
    std::string fn(name);
    memblock *mb = (memblock*)_GetPtr(fn);
    if(!mb)
    {
        LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
        if(vfile.mb.ptr)
        {
            mb = new memblock(vfile.mb.ptr, vfile.mb.size);
            vfile.src->Drop(fn.c_str()); // we own this ptr now, drop from storage
        }
        else
        {   // if the file was not found in the virtual file system, try loading from disk
            FILE *fh = fopen(name, "rb");
            if(!fh)
            {
                logerror("LoadFile failed: '%s'", name);
                return NULL;
            }

            fseek(fh, 0, SEEK_END);
            uint32 size = ftell(fh);
            rewind(fh);

            if(!size)
            {
                logerror("LoadFile skipped: '%s' (empty file)", name);
                return NULL;
            }

            mb = new memblock(new uint8[size], size);
            fread(mb->ptr, 1, size, fh);
            fclose(fh);
        }
        logdebug("LoadFile: '%s' -> "PTRFMT" [-> "PTRFMT"] size %u", name, mb, mb->ptr, mb->size);
    }

    _SetPtr(fn, (void*)mb);
    _IncRef((void*)mb, RESTYPE_MEMBLOCK);

    return mb;
}

memblock *ResourceMgr::LoadTextFile(char *name)
{
    std::string fn(name);
    memblock *mb = (memblock*)_GetPtr(fn);
    if(!mb)
    {
        LVPAFileStore::File vfile = _vfs.Get(fn.c_str());
        if(vfile.mb.ptr)
        {
            mb = new memblock(vfile.mb.ptr, vfile.mb.size);
            vfile.src->Drop(fn.c_str()); // we own this ptr now, drop from storage
        }
        else
        {   // if the file was not found in the virtual file system, try loading from disk
            FILE *fh = fopen(name, "r");
            if(!fh)
            {
                logerror("LoadTextFile failed: '%s'", name);
                return NULL;
            }

            fseek(fh, 0, SEEK_END);
            uint32 size = ftell(fh);
            rewind(fh);

            if(!size)
                return NULL;

            mb = new memblock(new uint8[size], size);
            uint8 *writeptr = mb->ptr;
            uint32 realsize = 0;
            while(!feof(fh))
            {
                int bytes = fread(writeptr, 1, 0x800, fh);
                writeptr += bytes;
                realsize += bytes;
            }
            memset(writeptr, 0, mb->size - realsize); // zero out remaining space
            mb->size = realsize;
            fclose(fh);
        }

        logdebug("LoadTextFile: '%s' -> "PTRFMT" [-> "PTRFMT"] size %u", name, mb, mb->ptr, mb->size);
    }

    _SetPtr(fn, (void*)mb);
    _IncRef((void*)mb, RESTYPE_MEMBLOCK);

    return mb;
}

std::string ResourceMgr::GetPropForFile(char *fn, char *prop)
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

void ResourceMgr::SetPropForFile(char *fn, char *prop, char *what)
{
    std::map<std::string,std::string>& fprop = _fprops[fn];
    std::map<std::string,std::string>::iterator it = fprop.find(prop);
    if(it != fprop.end())
        fprop.erase(it);
    fprop[prop] = what;
}

void ResourceMgr::LoadPropsInDir(char *dn)
{
    std::deque<std::string> fl = GetFileList(dn);
    for(std::deque<std::string>::iterator it = fl.begin(); it != fl.end(); it++)
    {
        if(FileGetExtension(*it) == ".prop")
            LoadPropFile((char*)it->c_str(), dn);
    }
}





// extern, global (since we aren't using singletons here)
ResourceMgr resMgr;
