
/** \file
Base module file, providing some engine core function bindings
*/

#include <falcon/engine.h>
#include "common.h"
#include "SharedDefines.h"
#include "Engine.h"
#include "AppFalcon.h"
#include "FalconBaseModule.h"
#include "SoundCore.h"
#include "ResourceMgr.h"
#include "TileLayer.h"
#include "ResourceMgr.h"
#include "LayerMgr.h"
#include "Objects.h"
#include "ObjectMgr.h"
#include "SoundCore.h"
#include "AsciiLevelParser.h"
#include "PropParser.h"
#include "LVPAFile.h"
#include "VFSHelper.h"
#include "VFSDir.h"
#include "VFSFile.h"
#include "Crc32.h"

// graphics/SDL related
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "SDL_func.h"

#include "UndefUselessCrap.h"


/*#
@module module_base base
@brief Contains bindings to core functions of the engine

This module provides bindings to engine core functions and classes,
shared across the game and the editor.

@beginmodule module_base
*/

Engine *g_engine_ptr_ = NULL;

void FalconBaseModule_SetEnginePtr(Engine *eng)
{
    g_engine_ptr_ = eng;
}


FALCON_FUNC fal_NullFunc(Falcon::VMachine *)
{
}

FALCON_FUNC fal_TrueFunc(Falcon::VMachine *vm)
{
    vm->retval(true);
}

FALCON_FUNC fal_FalseFunc(Falcon::VMachine *vm)
{
    vm->retval(false);
}

void forbidden_init(Falcon::VMachine *vm)
{
    throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
}

/*#
@function include_ex
@ingroup functions
@brief Include a falcon source file
@param file Filename to load, without path
@param nil Not used.
@param path Path where to load the file from

This function is similar to the original include() function present in Falcon,
but can only load source files (.fal).
Files inside container files can also be loaded by this function.
*/
FALCON_FUNC fal_include_ex( Falcon::VMachine *vm )
{
    Falcon::Item *i_file = vm->param(0);
    Falcon::Item *i_path = vm->param(2);
    if( !i_file || !i_file->isString()
        || (i_path != 0 && !(i_path->isString() || i_path->isNil()))
        )
    {
        throw new Falcon::ParamError(
            Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .origin(Falcon::e_orig_runtime)
            .extra( "S" ) );
    }

    Falcon::AutoCString cstr_fn(i_file->asString());
    Falcon::AutoCString cstr_path(i_path->asString());
    std::string fullpath(cstr_path.c_str());
    _FixFileName(fullpath);
    if(fullpath[fullpath.length() - 1] != '/')
        fullpath += '/';
    fullpath += cstr_fn.c_str();
    memblock *mb = resMgr.LoadTextFile((char*)fullpath.c_str());
    if(!mb)
    {
        vm->retval(false);
        return;
    }
    CRC32 crc;
    crc.Update(mb->ptr, mb->size);
    crc.Finalize();
    char crcbuf[8+2+1]; // 8 chars for CRC, 2 for '{}', 1 nul
    sprintf(crcbuf, "{%X}", crc.Result());
    std::string modName(crcbuf);
    modName += cstr_fn.c_str();

    // duplicate the string early, then drop the loaded file, forgetting about its content.
    // this is necessary if a file loads a file with the same name in the same path
    // (package.fal of package A loading package.fal of package B, for example)
    // otherwise, the script would load + call itself infinitely
    std::string dup((char*)mb->ptr);
    resMgr.Drop(mb, true);
    bool result = g_engine_ptr_->falcon->EmbedStringAsModule((char*)dup.c_str(), (char*)modName.c_str(), true, true);

    vm->retval(result);
}

/*#
@class Sound
@brief Sound clips that can be directly played.
@param filename : Name of the file to load, in directory 'sfx/' or a subdirectory.

The Sound class supports the following file formats: OGG, WAV.

If the file was not found or loading failed, @b nil is returned instead of a valid object.

Example use: Play file "sfx/button.ogg"
@code
    s = Sound("button.ogg")
    s.Play()
    // or alternatively
    Sound("button.ogg").Play()
@endcode

*/

fal_Sound::fal_Sound(SoundFile *sf)
: _snd(sf)
{
    DEBUG(ASSERT(_snd));
    _snd->ref++;
}

fal_Sound::~fal_Sound()
{
    _snd->ref--;
}

FALCON_FUNC fal_Sound_init( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
    Falcon::AutoCString cstr(vm->param(0)->asString());
    SoundFile *snd = sndCore.GetSound((char*)cstr.c_str());
    if(!snd)
    {
        vm->self().setNil();
        return;
    }
    vm->self().asObject()->setUserData(new fal_Sound(snd));
    snd->ref--;
}
/*#
@method Play Sound
@brief Plays a sound
@note A sound can be played multiple times, and overlapping.
*/
FALCON_FUNC fal_Sound_Play( Falcon::VMachine *vm )
{
    fal_Sound *self = (fal_Sound*)vm->self().asObject()->getUserData();
    self->GetSound()->Play();
}
/*#
@method Stop Sound
@brief Stops sound playback
@note If Play() was called multiple times, only the last started playback is stopped.
*/
FALCON_FUNC fal_Sound_Stop( Falcon::VMachine *vm )
{
    fal_Sound *self = (fal_Sound*)vm->self().asObject()->getUserData();
    self->GetSound()->Stop();
}

/*#
@method SetVolume Sound
@brief Adjusts sound volume [0..128]
@param volume Can be in range 0 (muted) to 128 (loudest)
@note This will change the volume of all sounds currently played by this object.
*/
FALCON_FUNC fal_Sound_SetVolume( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "N");
    uint32 vol = vm->param(0)->forceInteger();
    fal_Sound *self = (fal_Sound*)vm->self().asObject()->getUserData();
    self->GetSound()->SetVolume(vol);
}

/*#
@method GetVolume Sound
@brief Retrieve sound volume [0..128]
*/
FALCON_FUNC fal_Sound_GetVolume( Falcon::VMachine *vm )
{
    fal_Sound *self = (fal_Sound*)vm->self().asObject()->getUserData();
    vm->retval((Falcon::int64)self->GetSound()->GetVolume());
}

/*#
@method IsPlaying Sound
@brief Checks if this sound is playing on at least one channel
*/
FALCON_FUNC fal_Sound_IsPlaying( Falcon::VMachine *vm )
{
    fal_Sound *self = (fal_Sound*)vm->self().asObject()->getUserData();
    vm->retval(self->GetSound()->IsPlaying());
}
/*#
@function DbgBreak
@brief For debugging, should not be used

Set a breakpoint in C++ source in this function binding and break into the debugger when this fruntion is called, whewt!
*/
FALCON_FUNC fal_debug_break( Falcon::VMachine *vm )
{
    vm = NULL; // do nothing. set a breakpoint here and call the function from falcon
}

/*#
@function InvertSide
@param dir Direction or side
@brief Returns the opposite of a side or direction

Convenience function returning the opposite of DIRECTION_* and SIDE_* constants,
e.g. SIDE_LEFT gets SIDE_RIGHT, or DIRECTION_UPRIGHT gets DIRECTION_DOWNLEFT, and so on.
*/
FALCON_FUNC fal_InvertSide( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS(1);
    uint8 side = (uint8)vm->param(0)->forceInteger();
    vm->retval((Falcon::int64)InvertSide(side));
}

/*#
@method VFS AddContainer
@param filename Container (.lvpa) file to load
@param dir Directory to load into
@optparam overwrite If true, files in the virtual file tree will be replaced by those from the container file
@optparam preload 0 (default) - load only index; 1 - preload all files; 2 - preload only solid block
@brief Merges the contents of a container file into the virtual file system tree

After adding a container, its files and sub-directories can be accessed like if they were
in the directory specified.
It can then be mounted to other directories as well.
*/
FALCON_FUNC fal_VFS_AddContainer( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "S, S [, B [, N]]");
    if(!(vm->param(0)->isString() && vm->param(1)->isString()))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S, S [, B [, N]]") );
    }
    Falcon::String *fn = vm->param(0)->asString();
    Falcon::String *dir = vm->param(1)->asString();
    Falcon::AutoCString cfn(fn);
    bool overwrite = true;

    LVPALoadFlags mode = LVPALOAD_NONE;
    if(vm->paramCount() > 2)
    {
        overwrite = vm->param(2)->isTrue();

        if(vm->paramCount() > 3)
        {
            switch(vm->param(3)->forceInteger())
            {
                case 1: mode = LVPALOAD_ALL; break;
                case 2: mode = LVPALOAD_SOLID; break;
            }
        }
    }

    LVPAFile *lvpa = new LVPAFileReadOnly;
    if(!lvpa->LoadFrom(cfn.c_str(), mode))
    {
        delete lvpa;
        vm->retval(false);
        return;
    }

    Falcon::AutoCString cdir(dir);
    vm->retval(resMgr.vfs.AddContainer(lvpa, cdir.c_str(), true, overwrite));
}

/*#
@method VFS Clear
@brief Resets the virtual file system tree to its initial state

This drops all custom paths or containers added.
*/
FALCON_FUNC fal_VFS_Clear( Falcon::VMachine *vm )
{
    resMgr.vfs.Prepare(true);
}

/*#
@method VFS Reload
@brief Reloads all files contained in the virtual file system and refreshes files on disk

This should be called if files or directories on the file system were added or removed,
to refresh the virtual file system tree.
*/
FALCON_FUNC fal_VFS_Reload( Falcon::VMachine *vm )
{
    resMgr.vfs.Reload(true);
}

/*#
@method VFS GetDirList
@brief Returns all subdirectories in a directory
@return An array of strings with the names of the subdirectories of the given directory,
or @b nil if the directory doesn't exist.
*/
FALCON_FUNC fal_VFS_GetDirList( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S directory");
    Falcon::Item *arg = vm->param(0);
    if(!arg->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S") );
    }
    Falcon::String *str = arg->asString();
    Falcon::AutoCString cstr(arg->asString());
    VFSDir *vd = str->length() ? resMgr.vfs.GetDir(cstr.c_str()) : resMgr.vfs.GetDirRoot();
    if(!vd)
    {
        vm->retnil();
        return;
    }

    Falcon::CoreArray *arr = new Falcon::CoreArray(vd->_subdirs.size());
    for(VFSDirMap::iterator it = vd->_subdirs.begin(); it != vd->_subdirs.end(); it++)
    {
        arr->append(new Falcon::CoreString(it->second->name()));
    }

    vm->retval(arr);
}

/*#
@method VFS GetFileList
@brief Returns all files in a directory
@return An array of strings with the names of the files of the given directory,
or @b nil if the directory doesn't exist.
*/
FALCON_FUNC fal_VFS_GetFileList( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S directory");
    Falcon::Item *arg = vm->param(0);
    if(!arg->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S") );
    }
    Falcon::String *str = arg->asString();
    Falcon::AutoCString cstr(str);
    VFSDir *vd = str->length() ? resMgr.vfs.GetDir(cstr.c_str()) : resMgr.vfs.GetDirRoot();
    if(!vd)
    {
        vm->retnil();
        return;
    }

    Falcon::CoreArray *arr = new Falcon::CoreArray(vd->_subdirs.size());
    for(VFSFileMap::iterator it = vd->_files.begin(); it != vd->_files.end(); it++)
    {
        arr->append(new Falcon::CoreString(it->second->name()));
    }

    vm->retval(arr);
}

/*#
@method VFS HasFile
@brief Checks if a file exists in the VFS
@return True if the file is found, false if not
*/
FALCON_FUNC fal_VFS_HasFile( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S directory");
    Falcon::Item *arg = vm->param(0);
    if(!arg->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S") );
    }
    Falcon::AutoCString cstr(arg->asString());
    bool result = resMgr.vfs.GetFile(cstr.c_str());
    vm->retval(result);
}

/*#
@method VFS Merge
@brief Merges a directory into another directory
@param source The directory which will be mounted into target. Must exist.
@param target The directory where source will be merged into
@optparam force If true, create the target directory if not present
@return True if mounting was successful
*/
FALCON_FUNC fal_VFS_Merge( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "S, S [, B]");
    Falcon::Item *src_i = vm->param(0);
    Falcon::Item *targ_i = vm->param(1);
    Falcon::Item *force_i = vm->param(2);
    bool force = force_i && force_i->isTrue();
    if(!(src_i->isString() && targ_i->isString()))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S, S [, B]") );
    }
    Falcon::AutoCString csrc(src_i->asString());
    Falcon::AutoCString ctarg(targ_i->asString());

    VFSDir *targetdir = resMgr.vfs.GetDir(ctarg.c_str(), force);
    VFSDir *srcdir = resMgr.vfs.GetDir(csrc.c_str());
    if(targetdir && srcdir)
    {
        targetdir->merge(srcdir);
        vm->retval(true);
    }
    else
        vm->retval(false);
}

FALCON_FUNC fal_VFS_GetFileAsBuf( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S");
    Falcon::Item *i_fn = vm->param(0);
    if(!i_fn->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S") );
    }
    Falcon::AutoCString fn(i_fn->asString());
    VFSFile *vf = resMgr.vfs.GetFile((char*)fn.c_str());
    if(!vf)
    {
        vm->retnil();
        return;
    }

    uint32 size = vf->size();
    const uint8 *vbuf = vf->getBuf();
    if(!(size && vbuf))
    {
        vm->retnil();
        return;
    }
    Falcon::MemBuf_1 *mbuf = new Falcon::MemBuf_1(size);
    mbuf->length(size);
    memcpy(mbuf->data(), vbuf, size);

    vm->retval(mbuf);
}

FALCON_FUNC fal_VFS_AddBufAsFile( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "S, Buf");
    Falcon::Item *i_fn = vm->param(0);
    Falcon::Item *i_mbuf = vm->param(1);
    if(!(i_fn->isString() && i_mbuf->isMemBuf()))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S, Buf") );
    }
    Falcon::AutoCString fn(i_fn->asString());
    Falcon::MemBuf *mbuf = i_mbuf->asMemBuf();

    // figure out directory from full file name
    std::string dirname(fn.c_str());
    uint32 pathend = dirname.find_last_of("/\\");
    if(pathend != std::string::npos)
        dirname = dirname.substr(0, pathend);

    VFSDir *vdir = resMgr.vfs.GetDir(dirname.c_str(), true);
    VFSFileMem *vf = new VFSFileMem(fn.c_str(), mbuf->data(), mbuf->size(), true); // copy
    vm->retval(vdir->add(vf, true));
    --(vf->ref);
}

fal_Surface::fal_Surface(const Falcon::CoreClass* generator)
: Falcon::FalconObject( generator )
{
}

bool fal_Surface::finalize()
{
    if(!adopted)
        SDL_FreeSurface(surface);
    return false; // this tells the GC to call the destructor
}

Falcon::FalconObject *fal_Surface::clone(void) const
{
    return NULL; // TODO: implement this!
}

Falcon::CoreObject *fal_Surface::factory( const Falcon::CoreClass *cls, void *user_data, bool )
{
    return new fal_Surface(cls);
}

bool fal_Surface::setProperty( const Falcon::String &prop, const Falcon::Item &value )
{
    return false;
}

bool fal_Surface::getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
{
    if(prop == "w")          { ret = surface->w; return true; }
    else if(prop == "h")     { ret = surface->h; return true; }
    else if(prop == "ptr")   { ret = (uint64)surface->pixels; return true; }

    return defaultProperty( prop, ret);
}

void fal_Surface::init( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "N, N");
    int32 w = (uint32)vm->param(0)->forceInteger();
    int32 h = (uint32)vm->param(1)->forceInteger();
    if(!(w > 0 && h > 0))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("Width and Height must be > 0!") );
    }
    fal_Surface *self = Falcon::dyncast<fal_Surface*>( vm->self().asObject() );
    SDL_Surface *vs = SDL_GetVideoSurface();
    self->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, vs->format->BitsPerPixel,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000); // TODO: fix this for big endian
    self->adopted = false;
}

FALCON_FUNC fal_Surface_Pixel( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "N, N, N");
    int32 x = (uint32)vm->param(0)->forceInteger();
    int32 y = (uint32)vm->param(1)->forceInteger();
    int32 c = (uint32)vm->param(2)->forceInteger();
    fal_Surface *self = Falcon::dyncast<fal_Surface*>( vm->self().asObject() );
    SDL_Surface *s = self->surface;
    if(x < s->w && y < s->h)
        SDLfunc_putpixel(s, x, y, c);
}

FALCON_FUNC fal_Surface_Rect( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(5, "N, N, N, N, N [, B]");
    SDL_Rect rect;
    rect.x = (uint32)vm->param(0)->forceInteger();
    rect.y = (uint32)vm->param(1)->forceInteger();
    rect.w = (uint32)vm->param(2)->forceInteger();
    rect.h = (uint32)vm->param(3)->forceInteger();
    uint32 c = (uint32)vm->param(4)->forceInteger();
    Falcon::Item *i_fill = vm->param(5);
    bool fill = i_fill && i_fill->isTrue();
    fal_Surface *self = Falcon::dyncast<fal_Surface*>( vm->self().asObject() );
    SDL_Surface *s = self->surface;
    if(fill)
        SDL_FillRect(s, &rect, c);
    else
        SDLfunc_drawRectangle(s, rect, c);
}


FALCON_FUNC fal_Surface_BlitTo( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "Surface [, A [, A [, B]]]]");
    Falcon::Item *i_surface = vm->param(0);
    if(!(i_surface->isObject() && i_surface->asObject()->derivedFrom("Surface")))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("Object is not a surface") );
    }
    SDL_Surface *src = (Falcon::dyncast<fal_Surface*>(vm->self().asObject()))->surface;
    SDL_Surface *dst = (Falcon::dyncast<fal_Surface*>(i_surface->asObject()))->surface;

    Falcon::Item *i_srcrect = vm->param(1);
    Falcon::Item *i_dstrect = vm->param(2);
    Falcon::Item *i_rawblit = vm->param(3);
    bool rawblit = i_rawblit && i_rawblit->asBoolean();

    Falcon::CoreArray *a_srcrect = NULL;
    Falcon::CoreArray *a_dstrect = NULL;

    SDL_Rect srcrect, dstrect;

    if(i_srcrect)
    {
        if(i_srcrect->isArray() && (a_srcrect = i_srcrect->asArray())->length() >= 4)
        {
            srcrect.x = a_srcrect->at(0).forceIntegerEx();
            srcrect.y = a_srcrect->at(1).forceIntegerEx();
            srcrect.w = a_srcrect->at(2).forceIntegerEx();
            srcrect.h = a_srcrect->at(3).forceIntegerEx();
        }
        else if(i_srcrect->isNil())
        {
            i_srcrect = NULL; // speeds up check below
        }
        else
        {
            throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
                .extra("Srcrect must be array of length >= 4") );
        }
    }

    if(i_dstrect)
    {
        if(i_dstrect->isArray() && (a_dstrect = i_dstrect->asArray())->length() >= 2)
        {
            dstrect.x = a_dstrect->at(0).forceIntegerEx();
            dstrect.y = a_dstrect->at(1).forceIntegerEx();
            // w and h are ignored by SDL
        }
        else if(i_dstrect->isNil())
        {
            i_dstrect = NULL; // speeds up check below
        }
        else
        {
            throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
                .extra("Dstpos must be array of length >= 2") );
        }
    }

    if(rawblit)
    {
        uint8 oalpha = src->format->alpha;
        uint8 oflags = src->flags;
        SDL_SetAlpha(src, 0, 0); 
        SDL_BlitSurface(src, i_srcrect ? &srcrect : NULL, dst, i_dstrect ? &dstrect : NULL);
        src->format->alpha = oalpha;
        src->flags = oflags;
    }
    else
    {
        SDL_BlitSurface(src, i_srcrect ? &srcrect : NULL, dst, i_dstrect ? &dstrect : NULL);
    }
}

FALCON_FUNC fal_Surface_Write( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(4, "x, y, Font, S");
    Falcon::Item *i_font = vm->param(2);
    Falcon::Item *i_str = vm->param(3);
    if(!(i_str->isString() && i_font->isOfClass("Font")))
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("x, y, Font, S") );
    }

    uint32 x = (uint32)vm->param(0)->forceInteger();
    uint32 y = (uint32)vm->param(1)->forceInteger();
    gcn::SDLGraphics *gfx = (gcn::SDLGraphics*)g_engine_ptr_->GetGcnGfx();
    gcn::Font *font = ((fal_Font*)(i_font->asObject()->getUserData()))->GetFont();
    SDL_Surface *surf = Falcon::dyncast<fal_Surface*>(vm->self().asObject())->surface;
    Falcon::AutoCString cstr(i_str->asString());
    
    SDL_Surface *orig = gfx->getTarget();
    gfx->setTarget(surf);
    gfx->setFont(font);
    gfx->_beginDraw();
    gfx->drawText(cstr.c_str(), x, y);
    gfx->_endDraw();
    gfx->setTarget(orig);
}

FALCON_FUNC fal_Music_GetVolume(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(sndCore.GetMusicVolume()));
}

FALCON_FUNC fal_Music_SetVolume(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "Int volume");
    sndCore.SetMusicVolume(vm->param(0)->forceInteger());
}

FALCON_FUNC fal_Music_Play(Falcon::VMachine *vm)
{
    Falcon::Item *itm = vm->param(0);
    if(itm && !itm->isNil())
    {
        Falcon::AutoCString cstr(vm->param(0)->asString());
        sndCore.PlayMusic((char*)cstr.c_str());
    }
    else
    {
        sndCore.PlayMusic(NULL); // just unpause
    }
}

FALCON_FUNC fal_Music_Pause(Falcon::VMachine *vm)
{
    sndCore.PauseMusic();
}

FALCON_FUNC fal_Music_Stop(Falcon::VMachine *vm)
{
    sndCore.StopMusic();
}

FALCON_FUNC fal_Music_IsPlaying(Falcon::VMachine *vm)
{
    sndCore.IsPlayingMusic();
}

FALCON_FUNC fal_Engine_GetTime(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int64(g_engine_ptr_->GetCurFrameTime()));
}

FALCON_FUNC fal_Engine_LoadLevel(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1,"S filename");
    Falcon::AutoCString cstr(vm->param(0)->asString());
    AsciiLevel *level = LoadAsciiLevel((char*)cstr.c_str());
    if(!level)
    {
        throw new EngineError( Falcon::ErrorParam( Falcon::e_nofile ) );
    }
    g_engine_ptr_->_GetLayerMgr()->LoadAsciiLevel(level);
}

FALCON_FUNC fal_Engine_Exit(Falcon::VMachine *vm)
{
    g_engine_ptr_->SetQuit(true);
}

FALCON_FUNC fal_Engine_LoadPropFile(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S filename");
    Falcon::AutoCString cstr(vm->param(0)->asString());
    LoadPropFile((char*)cstr.c_str());
}

FALCON_FUNC fal_Engine_CreateCollisionMap(Falcon::VMachine *vm)
{
    bool update = false;
    if(vm->paramCount() >= 1)
        update = vm->param(0)->asBoolean();

    LayerMgr *lm = g_engine_ptr_->_GetLayerMgr();
    lm->CreateCollisionMap();
    if(update)
        lm->UpdateCollisionMap();
}

FALCON_FUNC fal_Engine_UpdateCollisionMap(Falcon::VMachine *vm)
{
    LayerMgr *lm = g_engine_ptr_->_GetLayerMgr();
    if(!lm->HasCollisionMap())
    {
        throw new EngineError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "CollisionMap not created" ) );
    }
    if(!vm->paramCount())
    {
        lm->UpdateCollisionMap();
    }
    else if(vm->paramCount() >= 2)
    {
        uint32 x = vm->param(0)->forceIntegerEx();
        uint32 y = vm->param(1)->forceIntegerEx();
        lm->UpdateCollisionMap(x,y);
    }
    else
    {
        throw new Falcon::ParamError( Falcon::ErrorParam( Falcon::e_missing_params ).
            extra( "No args or N,N" ) );
    }
}

FALCON_FUNC fal_Engine_Reset(Falcon::VMachine *vm)
{
    g_engine_ptr_->SetReset();
}

FALCON_FUNC fal_Engine_JoystickCount(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)Engine::GetJoystickCount());
}

FALCON_FUNC fal_Engine_JoystickInfo(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "I");
    uint32 id = vm->param(0)->forceIntegerEx();
    SDL_Joystick *jst = Engine::GetJoystick(id);
    if(jst)
    {
        Falcon::CoreArray *ca = new Falcon::CoreArray(5);
        Falcon::CoreString *jname = new Falcon::CoreString(SDL_JoystickName(id));
        ca->append(jname);
        ca->append((Falcon::int64)SDL_JoystickNumAxes(jst));
        ca->append((Falcon::int64)SDL_JoystickNumButtons(jst));
        ca->append((Falcon::int64)SDL_JoystickNumHats(jst));
        ca->append((Falcon::int64)SDL_JoystickNumBalls(jst));
        vm->retval(ca);
    }
    else
        vm->retnil();
}

FALCON_FUNC fal_Screen_GetLayer(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS(1);
    uint32 layerId = vm->param(0)->forceInteger();
    TileLayer *layer = g_engine_ptr_->_GetLayerMgr()->GetLayer(layerId);
    if(layer)
    {
        Falcon::CoreClass *cls = vm->findWKI("TileLayer")->asClass();
        vm->retval(new fal_TileLayer(cls, layer));
    }
    else
        vm->retnil();
}

FALCON_FUNC fal_Screen_GetSize(Falcon::VMachine *vm)
{
    Falcon::CoreArray *arr = new Falcon::CoreArray(2);
    arr->append((Falcon::int32)g_engine_ptr_->GetResX());
    arr->append((Falcon::int32)g_engine_ptr_->GetResY());
    vm->retval(arr);
}

FALCON_FUNC fal_Screen_GetWidth(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)g_engine_ptr_->GetResX());
}

FALCON_FUNC fal_Screen_GetHeight(Falcon::VMachine *vm)
{
    vm->retval((Falcon::int64)g_engine_ptr_->GetResY());
}

FALCON_FUNC fal_Screen_SetMode(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(4, "I, I, B, B");
    uint32 x = vm->param(0)->forceIntegerEx();
    uint32 y = vm->param(1)->forceIntegerEx();
    bool fs = vm->param(2)->isTrue();
    bool resiz = vm->param(3)->isTrue();
    SDL_Surface *surface = g_engine_ptr_->GetSurface();
    uint32 flags = (surface ? surface->flags & ~(SDL_RESIZABLE | SDL_FULLSCREEN) : 0);
    if(fs)
        flags |= SDL_FULLSCREEN;
    if(resiz)
        flags |= SDL_RESIZABLE;
    g_engine_ptr_->InitScreen(x, y, g_engine_ptr_->GetBPP(), flags);
}

FALCON_FUNC fal_Screen_SetBGColor(Falcon::VMachine *vm)
{
    if(!vm->paramCount())
    {
        g_engine_ptr_->SetDrawBG(false);
        return;
    }

    FALCON_REQUIRE_PARAMS_EXTRA(3, "I, I, I");
    
    uint8 r = uint8(vm->param(0)->forceInteger());
    uint8 g = uint8(vm->param(1)->forceInteger());
    uint8 b = uint8(vm->param(2)->forceInteger());
    g_engine_ptr_->SetBGColor(r,g,b);
    g_engine_ptr_->SetDrawBG(true);
}

FALCON_FUNC fal_Screen_IsResizable(Falcon::VMachine *vm)
{
    vm->retval(g_engine_ptr_->IsResizable());
}

FALCON_FUNC fal_Screen_IsFullscreen(Falcon::VMachine *vm)
{
    vm->retval(g_engine_ptr_->IsFullscreen());
}

FALCON_FUNC fal_Screen_GetLayerSize(Falcon::VMachine *vm)
{
    vm->retval((int64)g_engine_ptr_->_GetLayerMgr()->GetMaxDim());
}

FALCON_FUNC fal_Screen_GetTileInfo(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(2, "N, N");
    uint32 x = vm->param(0)->forceIntegerEx();
    uint32 y = vm->param(1)->forceIntegerEx();
    LayerMgr *lm = g_engine_ptr_->_GetLayerMgr();
    if(!lm->GetInfoLayer())
    {
        throw new EngineError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "TileInfoLayer not created" ) );
    }
    uint32 m = lm->GetMaxDim();
    if( !(x < m && y < m) )
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ) );
    }
    vm->retval((int64)lm->GetTileInfo(x,y));
}

FALCON_FUNC fal_Screen_SetTileInfo(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "N, N, N");
    uint32 x = vm->param(0)->forceIntegerEx();
    uint32 y = vm->param(1)->forceIntegerEx();
    uint32 info = vm->param(2)->forceIntegerEx();
    LayerMgr *lm = g_engine_ptr_->_GetLayerMgr();
    if(!lm->GetInfoLayer())
    {
        throw new EngineError( Falcon::ErrorParam( Falcon::e_undef_state ).
            extra( "TileInfoLayer not created" ) );
    }
    uint32 m = lm->GetMaxDim();
    if( !(x < m && y < m) )
    {
        throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_arracc ) );
    }
    lm->SetTileInfo(x,y,info);
}

FALCON_FUNC fal_Screen_CreateInfoLayer(Falcon::VMachine *vm)
{
    LayerMgr *lm = g_engine_ptr_->_GetLayerMgr();
    lm->CreateInfoLayer();
}

FALCON_FUNC fal_Screen_GetSurface(Falcon::VMachine *vm)
{
    Falcon::CoreClass *cls = vm->findWKI("Surface")->asClass();
    fal_Surface *fs = Falcon::dyncast<fal_Surface*>(fal_Surface::factory(cls, NULL, false));
    fs->surface = g_engine_ptr_->GetSurface();
    fs->adopted = true;
    vm->retval(fs);
}

fal_Font::~fal_Font()
{
    delete _font;
}

FALCON_FUNC fal_Font_init( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S image, S infofile");
    Falcon::AutoCString imgfile(vm->param(0)->asString());
    Falcon::AutoCString infofile(vm->param(1)->asString());
    gcn::Font *fnt = g_engine_ptr_->LoadFont(infofile.c_str(), imgfile.c_str());
    if(!fnt)
    {
        vm->self().setNil();
        return;
    }
    vm->self().asObject()->setUserData(new fal_Font(fnt));
}

FALCON_FUNC fal_Font_GetWidth( Falcon::VMachine *vm )
{
    Falcon::Item *itm = vm->param(0);
    if(!(itm && itm->isString()))
    {
        throw new Falcon::ParamError( Falcon::ErrorParam( itm ? Falcon::e_param_type : Falcon::e_missing_params ).
            extra( "S" ) );
    }
    Falcon::String *s = vm->param(0)->asString();
    gcn::Font *font = ((fal_Font*)(vm->self().asObject()->getFalconData()))->GetFont();
    Falcon::AutoCString cstr(s);

    vm->retval((int64)font->getWidth(cstr.c_str()));
}

FALCON_FUNC fal_Font_GetHeight( Falcon::VMachine *vm )
{
    gcn::Font *font = ((fal_Font*)(vm->self().asObject()->getFalconData()))->GetFont();
    vm->retval((int64)font->getHeight());
}

FALCON_FUNC fal_Color( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(3, "I, I, I [, I]")
    Falcon::Item *i_alpha = vm->param(3);
    SDL_PixelFormat *fmt = g_engine_ptr_->GetSurface()->format;
    uint8 r = vm->param(0)->forceInteger();
    uint8 g = vm->param(1)->forceInteger();
    uint8 b = vm->param(2)->forceInteger();
    if(i_alpha)
        vm->retval((Falcon::int64)SDL_MapRGB(fmt,r,g,b));
    else
        vm->retval((Falcon::int64)SDL_MapRGBA(fmt,r,g,b, i_alpha->forceInteger()));
}

Falcon::Module *FalconBaseModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("BaseModule");

    Falcon::Symbol *symEngine = m->addSingleton("Engine");
    Falcon::Symbol *clsEngine = symEngine->getInstance();
    m->addClassMethod(clsEngine, "GetTime", fal_Engine_GetTime);
    m->addClassMethod(clsEngine, "LoadLevel", fal_Engine_LoadLevel);
    m->addClassMethod(clsEngine, "Exit", fal_Engine_Exit);
    m->addClassMethod(clsEngine, "LoadPropFile", fal_Engine_LoadPropFile);
    m->addClassMethod(clsEngine, "CreateCollisionMap", fal_Engine_CreateCollisionMap);
    m->addClassMethod(clsEngine, "UpdateCollisionMap", fal_Engine_UpdateCollisionMap);
    m->addClassMethod(clsEngine, "Reset", fal_Engine_Reset);
    m->addClassMethod(clsEngine, "JoystickCount", fal_Engine_JoystickCount);
    m->addClassMethod(clsEngine, "JoystickInfo", fal_Engine_JoystickInfo);

    Falcon::Symbol *symScreen = m->addSingleton("Screen");
    Falcon::Symbol *clsScreen = symScreen->getInstance();
    m->addClassMethod(clsScreen, "GetLayer", &fal_Screen_GetLayer);
    m->addClassMethod(clsScreen, "GetSize", &fal_Screen_GetSize);
    m->addClassMethod(clsScreen, "GetWidth", &fal_Screen_GetWidth);
    m->addClassMethod(clsScreen, "GetHeight", &fal_Screen_GetHeight);
    m->addClassMethod(clsScreen, "GetLayerSize", &fal_Screen_GetLayerSize);
    m->addClassMethod(clsScreen, "SetTileInfo", &fal_Screen_SetTileInfo);
    m->addClassMethod(clsScreen, "GetTileInfo", &fal_Screen_GetTileInfo);
    m->addClassMethod(clsScreen, "CreateInfoLayer", &fal_Screen_CreateInfoLayer);
    m->addClassMethod(clsScreen, "GetSurface", &fal_Screen_GetSurface);
    m->addClassMethod(clsScreen, "SetMode", &fal_Screen_SetMode);
    m->addClassMethod(clsScreen, "SetBGColor", &fal_Screen_SetBGColor);
    m->addClassMethod(clsScreen, "CanResize", &fal_Screen_IsResizable);
    m->addClassMethod(clsScreen, "IsFullscreen", &fal_Screen_IsFullscreen);

    Falcon::Symbol *symMusic = m->addSingleton("Music");
    Falcon::Symbol *clsMusic = symMusic->getInstance();
    m->addClassMethod(clsMusic, "Play", fal_Music_Play);
    m->addClassMethod(clsMusic, "Stop", fal_Music_Stop);
    m->addClassMethod(clsMusic, "Pause", fal_Music_Pause);
    m->addClassMethod(clsMusic, "SetVolume", fal_Music_SetVolume);
    m->addClassMethod(clsMusic, "GetVolume", fal_Music_GetVolume);
    m->addClassMethod(clsMusic, "IsPlaying", fal_Music_IsPlaying);

    Falcon::Symbol *clsSound = m->addClass("Sound", fal_Sound_init);
    clsSound->setWKS(true);
    m->addClassMethod(clsSound, "Play", fal_Sound_Play);
    m->addClassMethod(clsSound, "Stop", fal_Sound_Stop);
    m->addClassMethod(clsSound, "SetVolume", fal_Sound_SetVolume);
    m->addClassMethod(clsSound, "GetVolume", fal_Sound_GetVolume);
    m->addClassMethod(clsSound, "IsPlaying", fal_Sound_IsPlaying);

    Falcon::Symbol *clsFont = m->addClass("Font", fal_Font_init);
    m->addClassMethod(clsFont, "GetWidth", fal_Font_GetWidth);
    m->addClassMethod(clsFont, "GetHeight", fal_Font_GetHeight);
    clsFont->setWKS(true);

    Falcon::Symbol *symVFS = m->addSingleton("VFS");
    Falcon::Symbol *clsVFS = symVFS->getInstance();
    m->addClassMethod(clsVFS, "AddContainer", fal_VFS_AddContainer);
    m->addClassMethod(clsVFS, "Clear", fal_VFS_Clear);
    m->addClassMethod(clsVFS, "Reload", fal_VFS_Reload);
    m->addClassMethod(clsVFS, "GetDirList", fal_VFS_GetDirList);
    m->addClassMethod(clsVFS, "GetFileList", fal_VFS_GetFileList);
    m->addClassMethod(clsVFS, "HasFile", fal_VFS_HasFile);
    m->addClassMethod(clsVFS, "Merge", fal_VFS_Merge);
    m->addClassMethod(clsVFS, "AddBufAsFile", fal_VFS_AddBufAsFile);
    m->addClassMethod(clsVFS, "GetFileAsBuf", fal_VFS_GetFileAsBuf);

    Falcon::Symbol *clsSurface = m->addClass("Surface", fal_Surface::init);
    clsSurface->setWKS(true);
    clsSurface->getClassDef()->factory(&fal_Surface::factory);
    m->addClassMethod(clsSurface, "Pixel", fal_Surface_Pixel);
    m->addClassMethod(clsSurface, "Rect", fal_Surface_Rect);
    m->addClassMethod(clsSurface, "BlitTo", fal_Surface_BlitTo);
    m->addClassMethod(clsSurface, "Write", fal_Surface_Write);

    m->addExtFunc("include_ex", fal_include_ex);
    m->addExtFunc("DbgBreak", fal_debug_break);
    m->addExtFunc("InvertSide", fal_InvertSide);
    m->addExtFunc("color", fal_Color);

    m->addConstant("MAX_VOLUME", Falcon::int64(MIX_MAX_VOLUME));

    m->addConstant("EVENT_TYPE_KEYBOARD",        Falcon::int64(EVENT_TYPE_KEYBOARD));
    m->addConstant("EVENT_TYPE_JOYSTICK_BUTTON", Falcon::int64(EVENT_TYPE_JOYSTICK_BUTTON));
    m->addConstant("EVENT_TYPE_JOYSTICK_AXIS",   Falcon::int64(EVENT_TYPE_JOYSTICK_AXIS));
    m->addConstant("EVENT_TYPE_JOYSTICK_HAT",    Falcon::int64(EVENT_TYPE_JOYSTICK_HAT));

    m->addConstant("DIRECTION_NONE",      Falcon::int64(DIRECTION_NONE));
    m->addConstant("DIRECTION_UP",        Falcon::int64(DIRECTION_UP));
    m->addConstant("DIRECTION_UPLEFT",    Falcon::int64(DIRECTION_UPLEFT));
    m->addConstant("DIRECTION_LEFT",      Falcon::int64(DIRECTION_LEFT));
    m->addConstant("DIRECTION_DOWNLEFT",  Falcon::int64(DIRECTION_DOWNLEFT));
    m->addConstant("DIRECTION_DOWN",      Falcon::int64(DIRECTION_DOWN));
    m->addConstant("DIRECTION_DOWNRIGHT", Falcon::int64(DIRECTION_DOWNRIGHT));
    m->addConstant("DIRECTION_RIGHT",     Falcon::int64(DIRECTION_RIGHT));
    m->addConstant("DIRECTION_UPRIGHT",   Falcon::int64(DIRECTION_UPRIGHT));

    m->addConstant("SIDE_NONE",        Falcon::int64(SIDE_NONE));
    m->addConstant("SIDE_TOP",         Falcon::int64(SIDE_TOP));
    m->addConstant("SIDE_TOPLEFT",     Falcon::int64(SIDE_TOPLEFT));
    m->addConstant("SIDE_LEFT",        Falcon::int64(SIDE_LEFT));
    m->addConstant("SIDE_BOTTOMLEFT",  Falcon::int64(SIDE_BOTTOMLEFT));
    m->addConstant("SIDE_BOTTOM",      Falcon::int64(SIDE_BOTTOM));
    m->addConstant("SIDE_BOTTOMRIGHT", Falcon::int64(SIDE_BOTTOMRIGHT));
    m->addConstant("SIDE_RIGHT",       Falcon::int64(SIDE_RIGHT));
    m->addConstant("SIDE_TOPRIGHT",    Falcon::int64(SIDE_TOPRIGHT));
    m->addConstant("SIDE_ALL",         Falcon::int64(SIDE_ALL));
    m->addConstant("SIDE_FLAG_SOLID",  Falcon::int64(SIDE_FLAG_SOLID));

    // the SDL key bindings
    Falcon::Symbol *c_sdlk = m->addClass( "SDLK", &forbidden_init);
    m->addClassProperty( c_sdlk, "BACKSPACE" ).setInteger( SDLK_BACKSPACE );
    m->addClassProperty( c_sdlk, "TAB" ).setInteger( SDLK_TAB );
    m->addClassProperty( c_sdlk, "CLEAR" ).setInteger( SDLK_CLEAR );
    m->addClassProperty( c_sdlk, "RETURN" ).setInteger( SDLK_RETURN );
    m->addClassProperty( c_sdlk, "PAUSE" ).setInteger( SDLK_PAUSE );
    m->addClassProperty( c_sdlk, "ESCAPE" ).setInteger( SDLK_ESCAPE );
    m->addClassProperty( c_sdlk, "SPACE" ).setInteger( SDLK_SPACE );
    m->addClassProperty( c_sdlk, "EXCLAIM" ).setInteger( SDLK_EXCLAIM );
    m->addClassProperty( c_sdlk, "QUOTEDBL" ).setInteger( SDLK_QUOTEDBL );
    m->addClassProperty( c_sdlk, "HASH" ).setInteger( SDLK_HASH );
    m->addClassProperty( c_sdlk, "DOLLAR" ).setInteger( SDLK_DOLLAR );
    m->addClassProperty( c_sdlk, "AMPERSAND" ).setInteger( SDLK_AMPERSAND );
    m->addClassProperty( c_sdlk, "QUOTE" ).setInteger( SDLK_QUOTE );
    m->addClassProperty( c_sdlk, "LEFTPAREN" ).setInteger( SDLK_LEFTPAREN );
    m->addClassProperty( c_sdlk, "RIGHTPAREN" ).setInteger( SDLK_RIGHTPAREN );
    m->addClassProperty( c_sdlk, "ASTERISK" ).setInteger( SDLK_ASTERISK );
    m->addClassProperty( c_sdlk, "PLUS" ).setInteger( SDLK_PLUS );
    m->addClassProperty( c_sdlk, "COMMA" ).setInteger( SDLK_COMMA );
    m->addClassProperty( c_sdlk, "MINUS" ).setInteger( SDLK_MINUS );
    m->addClassProperty( c_sdlk, "PERIOD" ).setInteger( SDLK_PERIOD );
    m->addClassProperty( c_sdlk, "SLASH" ).setInteger( SDLK_SLASH );
    m->addClassProperty( c_sdlk, "0" ).setInteger( SDLK_0 );
    m->addClassProperty( c_sdlk, "1" ).setInteger( SDLK_1 );
    m->addClassProperty( c_sdlk, "2" ).setInteger( SDLK_2 );
    m->addClassProperty( c_sdlk, "3" ).setInteger( SDLK_3 );
    m->addClassProperty( c_sdlk, "4" ).setInteger( SDLK_4 );
    m->addClassProperty( c_sdlk, "5" ).setInteger( SDLK_5 );
    m->addClassProperty( c_sdlk, "6" ).setInteger( SDLK_6 );
    m->addClassProperty( c_sdlk, "7" ).setInteger( SDLK_7 );
    m->addClassProperty( c_sdlk, "8" ).setInteger( SDLK_8 );
    m->addClassProperty( c_sdlk, "9" ).setInteger( SDLK_9 );
    m->addClassProperty( c_sdlk, "COLON" ).setInteger( SDLK_COLON );
    m->addClassProperty( c_sdlk, "SEMICOLON" ).setInteger( SDLK_SEMICOLON );
    m->addClassProperty( c_sdlk, "LESS" ).setInteger( SDLK_LESS );
    m->addClassProperty( c_sdlk, "EQUALS" ).setInteger( SDLK_EQUALS );
    m->addClassProperty( c_sdlk, "GREATER" ).setInteger( SDLK_GREATER );
    m->addClassProperty( c_sdlk, "QUESTION" ).setInteger( SDLK_QUESTION );
    m->addClassProperty( c_sdlk, "AT" ).setInteger( SDLK_AT );
    m->addClassProperty( c_sdlk, "LEFTBRACKET" ).setInteger( SDLK_LEFTBRACKET );
    m->addClassProperty( c_sdlk, "BACKSLASH" ).setInteger( SDLK_BACKSLASH );
    m->addClassProperty( c_sdlk, "RIGHTBRACKET" ).setInteger( SDLK_RIGHTBRACKET );
    m->addClassProperty( c_sdlk, "CARET" ).setInteger( SDLK_CARET );
    m->addClassProperty( c_sdlk, "UNDERSCORE" ).setInteger( SDLK_UNDERSCORE );
    m->addClassProperty( c_sdlk, "BACKQUOTE" ).setInteger( SDLK_BACKQUOTE );
    m->addClassProperty( c_sdlk, "a" ).setInteger( SDLK_a );
    m->addClassProperty( c_sdlk, "b" ).setInteger( SDLK_b );
    m->addClassProperty( c_sdlk, "c" ).setInteger( SDLK_c );
    m->addClassProperty( c_sdlk, "d" ).setInteger( SDLK_d );
    m->addClassProperty( c_sdlk, "e" ).setInteger( SDLK_e );
    m->addClassProperty( c_sdlk, "f" ).setInteger( SDLK_f );
    m->addClassProperty( c_sdlk, "g" ).setInteger( SDLK_g );
    m->addClassProperty( c_sdlk, "h" ).setInteger( SDLK_h );
    m->addClassProperty( c_sdlk, "i" ).setInteger( SDLK_i );
    m->addClassProperty( c_sdlk, "j" ).setInteger( SDLK_j );
    m->addClassProperty( c_sdlk, "k" ).setInteger( SDLK_k );
    m->addClassProperty( c_sdlk, "l" ).setInteger( SDLK_l );
    m->addClassProperty( c_sdlk, "m" ).setInteger( SDLK_m );
    m->addClassProperty( c_sdlk, "n" ).setInteger( SDLK_n );
    m->addClassProperty( c_sdlk, "o" ).setInteger( SDLK_o );
    m->addClassProperty( c_sdlk, "p" ).setInteger( SDLK_p );
    m->addClassProperty( c_sdlk, "q" ).setInteger( SDLK_q );
    m->addClassProperty( c_sdlk, "r" ).setInteger( SDLK_r );
    m->addClassProperty( c_sdlk, "s" ).setInteger( SDLK_s );
    m->addClassProperty( c_sdlk, "t" ).setInteger( SDLK_t );
    m->addClassProperty( c_sdlk, "u" ).setInteger( SDLK_u );
    m->addClassProperty( c_sdlk, "v" ).setInteger( SDLK_v );
    m->addClassProperty( c_sdlk, "w" ).setInteger( SDLK_w );
    m->addClassProperty( c_sdlk, "x" ).setInteger( SDLK_x );
    m->addClassProperty( c_sdlk, "y" ).setInteger( SDLK_y );
    m->addClassProperty( c_sdlk, "z" ).setInteger( SDLK_z );
    m->addClassProperty( c_sdlk, "DELETE" ).setInteger( SDLK_DELETE );
    m->addClassProperty( c_sdlk, "KP0" ).setInteger( SDLK_KP0 );
    m->addClassProperty( c_sdlk, "KP1" ).setInteger( SDLK_KP1 );
    m->addClassProperty( c_sdlk, "KP2" ).setInteger( SDLK_KP2 );
    m->addClassProperty( c_sdlk, "KP3" ).setInteger( SDLK_KP3 );
    m->addClassProperty( c_sdlk, "KP4" ).setInteger( SDLK_KP4 );
    m->addClassProperty( c_sdlk, "KP5" ).setInteger( SDLK_KP5 );
    m->addClassProperty( c_sdlk, "KP6" ).setInteger( SDLK_KP6 );
    m->addClassProperty( c_sdlk, "KP7" ).setInteger( SDLK_KP7 );
    m->addClassProperty( c_sdlk, "KP8" ).setInteger( SDLK_KP8 );
    m->addClassProperty( c_sdlk, "KP9" ).setInteger( SDLK_KP9 );
    m->addClassProperty( c_sdlk, "KP_PERIOD" ).setInteger( SDLK_KP_PERIOD );
    m->addClassProperty( c_sdlk, "KP_DIVIDE" ).setInteger( SDLK_KP_DIVIDE );
    m->addClassProperty( c_sdlk, "KP_MULTIPLY" ).setInteger( SDLK_KP_MULTIPLY );
    m->addClassProperty( c_sdlk, "KP_MINUS" ).setInteger( SDLK_KP_MINUS );
    m->addClassProperty( c_sdlk, "KP_PLUS" ).setInteger( SDLK_KP_PLUS );
    m->addClassProperty( c_sdlk, "KP_ENTER" ).setInteger( SDLK_KP_ENTER );
    m->addClassProperty( c_sdlk, "KP_EQUALS" ).setInteger( SDLK_KP_EQUALS );
    m->addClassProperty( c_sdlk, "UP" ).setInteger( SDLK_UP );
    m->addClassProperty( c_sdlk, "DOWN" ).setInteger( SDLK_DOWN );
    m->addClassProperty( c_sdlk, "RIGHT" ).setInteger( SDLK_RIGHT );
    m->addClassProperty( c_sdlk, "LEFT" ).setInteger( SDLK_LEFT );
    m->addClassProperty( c_sdlk, "INSERT" ).setInteger( SDLK_INSERT );
    m->addClassProperty( c_sdlk, "HOME" ).setInteger( SDLK_HOME );
    m->addClassProperty( c_sdlk, "END" ).setInteger( SDLK_END );
    m->addClassProperty( c_sdlk, "PAGEUP" ).setInteger( SDLK_PAGEUP );
    m->addClassProperty( c_sdlk, "PAGEDOWN" ).setInteger( SDLK_PAGEDOWN );
    m->addClassProperty( c_sdlk, "F1" ).setInteger( SDLK_F1 );
    m->addClassProperty( c_sdlk, "F2" ).setInteger( SDLK_F2 );
    m->addClassProperty( c_sdlk, "F3" ).setInteger( SDLK_F3 );
    m->addClassProperty( c_sdlk, "F4" ).setInteger( SDLK_F4 );
    m->addClassProperty( c_sdlk, "F5" ).setInteger( SDLK_F5 );
    m->addClassProperty( c_sdlk, "F6" ).setInteger( SDLK_F6 );
    m->addClassProperty( c_sdlk, "F7" ).setInteger( SDLK_F7 );
    m->addClassProperty( c_sdlk, "F8" ).setInteger( SDLK_F8 );
    m->addClassProperty( c_sdlk, "F9" ).setInteger( SDLK_F9 );
    m->addClassProperty( c_sdlk, "F10" ).setInteger( SDLK_F10 );
    m->addClassProperty( c_sdlk, "F11" ).setInteger( SDLK_F11 );
    m->addClassProperty( c_sdlk, "F12" ).setInteger( SDLK_F12 );
    m->addClassProperty( c_sdlk, "F13" ).setInteger( SDLK_F13 );
    m->addClassProperty( c_sdlk, "F14" ).setInteger( SDLK_F14 );
    m->addClassProperty( c_sdlk, "F15" ).setInteger( SDLK_F15 );
    m->addClassProperty( c_sdlk, "NUMLOCK" ).setInteger( SDLK_NUMLOCK );
    m->addClassProperty( c_sdlk, "CAPSLOCK" ).setInteger( SDLK_CAPSLOCK );
    m->addClassProperty( c_sdlk, "SCROLLOCK" ).setInteger( SDLK_SCROLLOCK );
    m->addClassProperty( c_sdlk, "RSHIFT" ).setInteger( SDLK_RSHIFT );
    m->addClassProperty( c_sdlk, "LSHIFT" ).setInteger( SDLK_LSHIFT );
    m->addClassProperty( c_sdlk, "RCTRL" ).setInteger( SDLK_RCTRL );
    m->addClassProperty( c_sdlk, "LCTRL" ).setInteger( SDLK_LCTRL );
    m->addClassProperty( c_sdlk, "RALT" ).setInteger( SDLK_RALT );
    m->addClassProperty( c_sdlk, "LALT" ).setInteger( SDLK_LALT );
    m->addClassProperty( c_sdlk, "RMETA" ).setInteger( SDLK_RMETA );
    m->addClassProperty( c_sdlk, "LMETA" ).setInteger( SDLK_LMETA );
    m->addClassProperty( c_sdlk, "LSUPER" ).setInteger( SDLK_LSUPER );
    m->addClassProperty( c_sdlk, "RSUPER" ).setInteger( SDLK_RSUPER );
    m->addClassProperty( c_sdlk, "MODE" ).setInteger( SDLK_MODE );
    m->addClassProperty( c_sdlk, "HELP" ).setInteger( SDLK_HELP );
    m->addClassProperty( c_sdlk, "PRINT" ).setInteger( SDLK_PRINT );
    m->addClassProperty( c_sdlk, "SYSREQ" ).setInteger( SDLK_SYSREQ );
    m->addClassProperty( c_sdlk, "BREAK" ).setInteger( SDLK_BREAK );
    m->addClassProperty( c_sdlk, "MENU" ).setInteger( SDLK_MENU );
    m->addClassProperty( c_sdlk, "POWER" ).setInteger( SDLK_POWER );
    m->addClassProperty( c_sdlk, "EURO" ).setInteger( SDLK_EURO );

    return m;
}