#include "common.h"
#include "AppFalcon.h"

/*
Falcon::Module *AppBaseModule_create(void)
{
    Falcon::Module *module = new Falcon::Module;
    module->name( "AppBaseModule" );
    module->addConstant("LOADED_FROM_SOURCE", Falcon::int64(have_source ? 1 : 0));
    return module;
}
*/


AppFalcon::AppFalcon()
: rt(NULL), vm(NULL)
{
}

AppFalcon::~AppFalcon()
{
    if(rt)
        delete rt;
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
    rt = new Falcon::Runtime(&mloader);
    vm->launchAtLink(false);
    _LoadModules();
}



void AppFalcon::_LoadModules(void)
{
    vm->link( Falcon::core_module_init() );  // add the core module
}

/*
bool AppFalcon::Embed( const char *script_name )
{
    // just to cover the case setting this was forgotten
    ASSERT(_modulePath.length());

    try
    {
        // first of all, we need a module loader to load the script.
        // The parameter is the search path for where to search our module
        mloader.addSearchPath(_modulePath.c_str());

        // Allow the script to load iteratively other resources it may need.
        rt = new Falcon::Runtime(&mloader);
        bool have_source = true;
        try
        {
            rt->loadFile( script_name );
        }
        catch(Falcon::IoError*)
        {
            // unable to load .fal file, check if there is a .fam file
            // if we originally wanted to load a .fal
            uint32 len = strlen(script_name);
            if(!stricmp(script_name + len - 4, ".fal"))
            {
                std::string newname(script_name);
                newname[len - 1] = 'm'; // replace .fal L with M -> .fam
                rt->loadFile(newname.c_str()); // may throw again
                have_source = false; // it didnt throw IoError if we reached this line, means loading the file ws successful
            }
            else
                throw; // nothing to load
        }
        
        vm = new Falcon::VMachine;
        vm->launchAtLink(false);

        // Finally add our application modules
        _LoadModules();

        // try to link our module and its dependencies.
        // -- It may fail if there are some undefined symbols
        vm->link(rt);

        // we're ready to go. Still, we may fail if the script has not a main routine.
        vm->launch();
    }
    catch(Falcon::Error *err)
    {
        Falcon::AutoCString edesc( err->toString() );
        logerror("AppFalcon::Embed: %s", edesc.c_str());
        err->decref();
        return false;
    }

    return true;
}
*/

bool AppFalcon::EmbedStringAsModule(char *str, char *modName)
{
    if(!str || str[0] == '\0')
        return false;

    Falcon::ROStringStream input( str );

    Falcon::Module *m = NULL;

    try
    {
        m = mloader.loadSource( &input, modName, modName );
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
        rt->addModule(m);
        vm->link(rt);
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

std::string AppFalcon::ExecuteString(char *line)
{
    if(!line || line[0] == '\0')
        return "";

    DEBUG_LOG("AppFalcon:ExecuteString: %s", line);

    Falcon::ROStringStream input( line );
    Falcon::ModuleLoader stringloader;
    stringloader.compileTemplate(false);
    stringloader.compileInMemory(true);
    stringloader.alwaysRecomp(true);
    stringloader.saveModules(false);

    Falcon::Module *m = NULL;

    try
    {
        m = stringloader.loadSource( &input, "<AppFalcon::ExecuteString>", "_eval_" );

        bool ll = vm->launchAtLink();
        vm->launchAtLink(true);
        vm->link(m,false,true);
        vm->launchAtLink(ll);

    }
    catch(Falcon::Error *err)
    {
        Falcon::AutoCString edesc( err->toString() );
        logerror("AppFalcon::ExecuteString: %s", edesc.c_str());
        err->decref();

    }

    if(m)
    {
        vm->unlink(m);
        m->decref();
    }

    return std::string(); // error
}