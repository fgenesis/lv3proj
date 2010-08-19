
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
    std::string modName = cstr_fn.c_str();
    bool result = g_engine_ptr_->falcon->EmbedStringAsModule((char*)mb->ptr, (char*)modName.c_str(), true, true, false);

    vm->retval(result);


    // OLD CODE
    /*
    Falcon::Item *i_file = vm->param(0);
    Falcon::Item *i_enc = vm->param(1);
    Falcon::Item *i_path = vm->param(2);
    Falcon::Item *i_syms = vm->param(3);

    if( i_file == 0 || ! i_file->isString()
        || (i_syms != 0 && ! (i_syms->isDict() || i_syms->isNil())  )
        || (i_enc != 0 && !(i_enc->isString() || i_enc->isNil()) )
        || (i_path != 0 && !(i_path->isString() || i_path->isNil()) )
        )
    {
        throw new Falcon::ParamError(
            Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
            .origin(Falcon::e_orig_runtime)
            .extra( "S,[S],[S],[D]" ) );
    }

    // create the loader/runtime pair.
    Falcon::ModuleLoader cpl( i_path == 0 || i_path->isNil() ? vm->appSearchPath() : Falcon::String(*i_path->asString()) );
    cpl.delayRaise(true);
    cpl.compileTemplate(false);
    cpl.compileInMemory(true);
    cpl.alwaysRecomp(true);
    cpl.saveModules(false);
    Falcon::Runtime rt( &cpl, vm );
    rt.hasMainModule( false );

    // minimal config
    if ( i_enc != 0 && ! i_enc->isNil() )
    {
        cpl.sourceEncoding( *i_enc->asString() );
    }

    bool execAtLink = vm->launchAtLink();

    //! Copy the filename so to be sure to display it correctly in an eventual error.
    Falcon::String fileName = *i_file->asString();
    fileName.bufferize();

    // load and link
    try
    {
        rt.loadFile( fileName, false );
        vm->launchAtLink( i_syms == 0 || i_syms->isNil() );
        Falcon::LiveModule *lmod = vm->link( &rt );

        // shall we read the symbols?
        if( lmod != 0 && ( i_syms != 0 && i_syms->isDict() ) )
        {
            Falcon::CoreDict *dict = i_syms->asDict();

            // traverse the dictionary
            Falcon::Iterator iter( &dict->items() );
            while( iter.hasCurrent() )
            {
                // if the key is a string and a corresponding item is found...
                Falcon::Item *ival;
                if ( iter.getCurrentKey().isString() &&
                    ( ival = lmod->findModuleItem( *iter.getCurrentKey().asString() ) ) != 0 )
                {
                    // copy it locally
                    iter.getCurrent() = *ival;
                }
                else {
                    iter.getCurrent().setNil();
                }

                iter.next();
            }
        }

        // reset launch status
        vm->launchAtLink( execAtLink );
    }
    catch(Falcon::Error* err)
    {
        Falcon::CodeError *ce = new Falcon::CodeError( Falcon::ErrorParam( Falcon::e_loaderror, __LINE__ ).
            extra( fileName ) );

        ce->appendSubError(err);
        err->decref();

        // reset launch status
        vm->launchAtLink( execAtLink );
        throw ce;
    }
    */
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