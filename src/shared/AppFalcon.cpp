#include "common.h"
#include "AppFalcon.h"

// defined in falcon/compiler_module/compiler.cpp
Falcon::Module *falcon_compiler_module_init(void);


AppFalcon::AppFalcon()
: vm(NULL)
{
}

AppFalcon::~AppFalcon()
{
    if(vm)
        vm->finalize();
}

void AppFalcon::Init(void)
{
    vm = new Falcon::VMachine();
    mloader.compileTemplate(false);
    mloader.compileInMemory(true);
    mloader.alwaysRecomp(true);
    mloader.saveModules(false);
    vm->launchAtLink(false);
    _LoadModules();
}



void AppFalcon::_LoadModules(void)
{
    vm->link( Falcon::core_module_init() );  // add the core module
    vm->link( falcon_compiler_module_init() );
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
