#include "common.h"
#include "AppFalcon.h"

#include "FalconBaseModule.h"
#include "FalconObjectModule.h"

// defined in falcon/compiler_module/compiler.cpp
Falcon::Module *falcon_compiler_module_init(void);

// defined in falcon/confparser_module/confparser.cpp
Falcon::Module *falcon_confparser_module_init(void);

// defined in falcon/bufext_module/bufext.cpp
Falcon::Module *bufext_module_init(void);


AppFalcon::AppFalcon()
: vm(NULL)
{
    mloader.compileTemplate(false);
    mloader.compileInMemory(true);
    mloader.alwaysRecomp(true);
    mloader.saveModules(false);
    mloader.delayRaise(true);
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
    return initscript ? EmbedStringAsModule(initscript, "initscript", false, true) : true;
}

void AppFalcon::DeleteVM(void)
{
    if(vm)
    {
        vm->finalize();
        vm = NULL;
    }
}

void AppFalcon::_LinkModule(Falcon::Module *m)
{
    vm->link(m);
    m->decref();
}

void AppFalcon::_LoadModules(void)
{
    _LinkModule( Falcon::core_module_init() );  // add the core module
    _LinkModule( falcon_compiler_module_init() );
    _LinkModule( falcon_confparser_module_init() );
    _LinkModule( bufext_module_init() );
    _LinkModule( FalconBaseModule_create() );
    _LinkModule( FalconObjectModule_create() );
}

bool AppFalcon::EmbedStringAsModule(char *str, char *modName, bool throw_ /* = false */,
                                    bool launch /* = false */)
{
    if(!str || str[0] == '\0')
        return false;

    Falcon::StringStream input( str );
    Falcon::TranscoderEOL trans(&input, false);

    Falcon::Runtime rt(&mloader, vm);

    Falcon::Module *m = NULL;

    bool success = true;

    try
    {
        m = mloader.loadSource( &trans, modName, modName );

        rt.addModule(m);
        m->decref(); // we can abandon our reference to the script module
        Falcon::LiveModule *livemod = vm->link(&rt);
        if(launch)
        {
            Falcon::Item *mainItem = livemod->findModuleItem("__main__");
            if(mainItem)
            {
                vm->callFrame(*mainItem, 0);
                vm->execFrame();
            }
        }
    }
    catch(Falcon::Error *err)
    {
        if(throw_)
        {
            Falcon::CodeError *ce = new Falcon::CodeError( Falcon::ErrorParam( Falcon::e_loaderror, __LINE__ ).
                extra( modName ) );

            ce->appendSubError(err);
            err->decref();

            throw ce;
        }
        Falcon::AutoCString edesc( err->toString() );
        logerror("AppFalcon::EmbedStringAsModule(%s): %s", modName, edesc.c_str());
        err->decref();
        success = false;
    }

    mloader.compiler().reset();

    return success;
}
