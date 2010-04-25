#include "common.h"
#include "AppFalcon.h"

#include "FalconBaseModule.h"
#include "FalconObjectModule.h"

// defined in falcon/compiler_module/compiler.cpp
Falcon::Module *falcon_compiler_module_init(void);


AppFalcon::AppFalcon(Engine *e)
: vm(NULL)
{
    mloader.compileTemplate(false);
    mloader.compileInMemory(true);
    mloader.alwaysRecomp(true);
    mloader.saveModules(false);
    FalconObjectModule_SetEnginePtr(e);
}

AppFalcon::~AppFalcon()
{
    DeleteVM();
}

bool AppFalcon::Init(char *initscript /* = NULL */)
{
    DeleteVM();
    vm = new Falcon::VMachine();
    vm->launchAtLink(false);
    _LoadModules();
    return initscript ? EmbedStringAsModule(initscript, "initscript") : true;
}

void AppFalcon::DeleteVM(void)
{
    if(vm)
    {
        vm->finalize();
        vm = NULL;
    }
}

void AppFalcon::_LoadModules(void)
{
    vm->link( Falcon::core_module_init() );  // add the core module
    vm->link( falcon_compiler_module_init() );
    vm->link( FalconBaseModule_create() );
    vm->link( FalconObjectModule_create() );
}

bool AppFalcon::EmbedStringAsModule(char *str, char *modName)
{
    if(!str || str[0] == '\0')
        return false;

    Falcon::StringStream input( str );
    Falcon::TranscoderEOL trans(&input, false);

    Falcon::Runtime rt(&mloader);

    Falcon::Module *m = NULL;

    try
    {
        m = mloader.loadSource( &trans, modName, modName );
    }
    catch(Falcon::Error *err)
    {
        Falcon::AutoCString edesc( err->toString() );
        logerror("AppFalcon::EmbedStringAsModule(%s): %s", modName, edesc.c_str());
        err->decref();
        return false;
    }

    try
    {
        rt.addModule(m);
        vm->link(&rt);
        vm->launch();
    }
    catch(Falcon::Error *err)
    {
        Falcon::AutoCString edesc( err->toString() );
        logerror("AppFalcon::EmbedStringAsModule(%s): %s", modName, edesc.c_str());
        err->decref();
        return false;
    }

    return true;
}
