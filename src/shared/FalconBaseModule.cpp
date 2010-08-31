
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
@method VFS AddPath
@param path Relative directory name
@brief Merges a directory into the virtual file system tree

After adding a path, its files and sub-directories can be accessed like if they were
in the root directory.
*/
FALCON_FUNC fal_VFS_AddPath( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S");
    if(!vm->param(0)->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S") );
    }
    Falcon::String *path = vm->param(0)->asString();
    Falcon::AutoCString cstr(path);
    vm->retval(resMgr.vfs.AddPath(cstr.c_str()));
}

/*#
@method VFS AddContainer
@param filename Container (.lvpa) file to load
@brief Merges the contents of a container file into the virtual file system tree

After adding a container, its files and sub-directories can be accessed like if they were
in the root directory on the file system.
*/
FALCON_FUNC fal_VFS_AddContainer( Falcon::VMachine *vm )
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "S [, N]");
    if(!vm->param(0)->isString())
    {
        throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .extra("S [, N]") );
    }
    Falcon::String *path = vm->param(0)->asString();
    Falcon::AutoCString cstr(path);

    LVPALoadFlags mode = LVPALOAD_NONE;
    if(vm->paramCount() > 1)
    {
        switch(vm->param(0)->forceInteger())
        {
            case 1: mode = LVPALOAD_ALL; break;
            case 2: mode = LVPALOAD_SOLID; break;
        }
    }

    LVPAFile *lvpa = new LVPAFileReadOnly;
    if(!lvpa->LoadFrom(cstr.c_str(), mode))
    {
        delete lvpa;
        vm->retval(false);
        return;
    }

    vm->retval(resMgr.vfs.AddContainer(lvpa, true));
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
    // TODO: make this reload mounted subdirs too...?
    resMgr.vfs.LoadFileSysRoot();
    resMgr.vfs.Reload();
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
    Falcon::AutoCString cstr(arg->asString());
    VFSDir *vd = resMgr.vfs.GetDir(cstr.c_str());
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
    Falcon::AutoCString cstr(arg->asString());
    VFSDir *vd = resMgr.vfs.GetDir(cstr.c_str());
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


Falcon::Module *FalconBaseModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("BaseModule");

    Falcon::Symbol *clsSound = m->addClass("Sound", fal_Sound_init);
    m->addClassMethod(clsSound, "Play", fal_Sound_Play);
    m->addClassMethod(clsSound, "Stop", fal_Sound_Stop);
    m->addClassMethod(clsSound, "SetVolume", fal_Sound_SetVolume);
    m->addClassMethod(clsSound, "GetVolume", fal_Sound_GetVolume);
    m->addClassMethod(clsSound, "IsPlaying", fal_Sound_IsPlaying);

    Falcon::Symbol *symVFS = m->addSingleton("VFS");
    Falcon::Symbol *clsVFS = symVFS->getInstance();
    m->addClassMethod(clsVFS, "AddPath", fal_VFS_AddPath);
    m->addClassMethod(clsVFS, "AddContainer", fal_VFS_AddContainer);
    m->addClassMethod(clsVFS, "Clear", fal_VFS_Clear);
    m->addClassMethod(clsVFS, "Reload", fal_VFS_Reload);
    m->addClassMethod(clsVFS, "GetDirList", fal_VFS_GetDirList);
    m->addClassMethod(clsVFS, "GetFileList", fal_VFS_GetFileList);
    m->addClassMethod(clsVFS, "HasFile", fal_VFS_HasFile);

    Falcon::Symbol *clsSurface = m->addClass("Surface", fal_Surface::init);
    clsSurface->setWKS(true);
    clsSurface->getClassDef()->factory(&fal_Surface::factory);
    m->addClassMethod(clsSurface, "Pixel", fal_Surface_Pixel);
    m->addClassMethod(clsSurface, "BlitTo", fal_Surface_BlitTo);

    m->addExtFunc("include_ex", fal_include_ex);
    m->addExtFunc("DbgBreak", fal_debug_break);
    m->addExtFunc("InvertSide", fal_InvertSide);

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

    return m;
}