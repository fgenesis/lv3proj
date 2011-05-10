#include "common.h"
#include "AppFalcon.h"
#include <falcon/stream.h>

#include "FalconBaseModule.h"
#include "FalconObjectModule.h"

// defined in falcon/compiler_module/compiler.cpp
Falcon::Module *falcon_compiler_module_init(void);

// defined in falcon/confparser_module/confparser.cpp
Falcon::Module *falcon_confparser_module_init(void);

// defined in falcon/bufext_module/bufext.cpp
Falcon::Module *bufext_module_init(void);


class MyStream: public Falcon::Stream
{
    AppFalcon *m_app;
    Falcon::Stream *m_extra;
    int m_col;

    // This is where the buffer will be stored.
    Falcon::String m_buffer;

    // we're using this as a proxy for text writes.
    void internalWrite();

protected:
    virtual Falcon::int64 seek( Falcon::int64 pos, e_whence w );

public:
    MyStream( AppFalcon *app, Falcon::Stream *extra, int col):
      Stream( t_stream ),
          m_app( app ),
          m_extra(extra),
          m_col(col)
      {}

      // We don't really need to implement all of those;
      // as we want to reimplement output streams, we'll just
      // set "unsupported" where we don't want to provide support.
      bool writeString( const Falcon::String &source, Falcon::uint32 begin=0, Falcon::uint32 end = Falcon::csh::npos );
      virtual bool close();
      virtual Falcon::int32 read( void *buffer, Falcon::int32 size );
      virtual Falcon::int32 write( const void *buffer, Falcon::int32 size );
      virtual Falcon::int64 tell();
      virtual bool truncate( Falcon::int64 pos=-1 );
      virtual Falcon::int32 readAvailable( Falcon::int32 msecs_timeout );
      virtual Falcon::int32 writeAvailable( Falcon::int32 msecs_timeout );
      virtual Falcon::int64 lastError() const;
      virtual bool get( Falcon::uint32 &chr );
      virtual bool put( Falcon::uint32 chr );
};

// Byte oriented write.
// Scripts don't normally use standards VM streams to perform
// binary write, but there's no reason not to support it.
// We don't prefix nor translate, as this shall be some binary data
// as i.e. images.
Falcon::int32 MyStream::write( const void *buffer, Falcon::int32 size )
{
    //const char *buf = (const char *)buffer;
    //m_out->write( buf, size );
    //return size;
    status( t_unsupported );
    return -1;
}

// Text oriented write
bool MyStream::writeString( const Falcon::String &source, Falcon::uint32 begin, Falcon::uint32 end )
{
    if ( begin != 0 || end <= source.length() )
    {
        Falcon::String sub( source, begin, end );
        m_buffer += sub;
    }
    else
        m_buffer += source;

    m_extra->writeString(source, begin, end);
    m_extra->flush();

    internalWrite();

    return true;
}

// char oriented write (used mainly by transcoders)
bool MyStream::put( Falcon::uint32 chr )
{
    m_buffer += chr;
    internalWrite();
    m_extra->put(chr);
    m_extra->flush(); // ehm... really?
    return true;
}

Falcon::int64 MyStream::seek( Falcon::int64 pos, e_whence w )
{
    status( t_unsupported );
    return -1;
}


bool MyStream::close()
{
    return true;
}

Falcon::int32 MyStream::read( void *buffer, Falcon::int32 size )
{
    status( t_unsupported );
    return -1;
}

Falcon::int64 MyStream::tell()
{
    return 0;
}

bool MyStream::truncate( Falcon::int64 pos )
{
    status( t_unsupported );
    return false;
}

Falcon::int32 MyStream::readAvailable( Falcon::int32 msecs_timeout )
{
    status( t_unsupported );
    return -1;
}

Falcon::int32 MyStream::writeAvailable( Falcon::int32 msecs_timeout )
{
    return 1;
}

Falcon::int64 MyStream::lastError() const
{
    return 0;
}

bool MyStream::get( Falcon::uint32 &chr )
{
    status( t_unsupported );
    return false;
}

// WARNING: Not thread safe!
// As only debug log with loglevel >= 2 is used from other threads,
// and this is skipped below, things are safe, for now.
static void _CallVMWrite(Falcon::VMachine *vm, Falcon::String *str, Falcon::int32 col)
{
    Falcon::Item *item = vm->findGlobalItem("PrintCallback");
    if(item && item->isCallable())
    {
        try
        {
            str->bufferize();
            vm->pushParam(str);
            vm->pushParam(col);
            vm->callItemAtomic(*item, 2);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("AppFalcon::_CallVMWrite: %s", edesc.c_str());
            err->decref();
        }
    }
}


// this performs the real output
void MyStream::internalWrite()
{
    // we'll scan for each line in the buffer...
    Falcon::uint32 pos1 = 0;
    Falcon::uint32 pos2 = m_buffer.find( "\n" );

    while( pos2 != Falcon::String::npos )
    {
        _CallVMWrite(m_app->GetVM(), new Falcon::CoreString(m_buffer.subString( pos1, pos2 )), m_col); // TODO: CoreString really needed?
        pos1 = pos2 + 1;
        pos2 = m_buffer.find( "\n", pos1 );
    }

    // trim the buffer
    m_buffer = m_buffer.subString( pos1 );
}

static void log_falcon_bridge(const char *s, int c, int loglevel, void *user)
{
    // we don't want to see engine debug log (that is, logdebug() and logdev())
    // also mind the above note about thread safety.
    if(loglevel >= 2)
        return;

    Falcon::VMachine *vm = (Falcon::VMachine*)user;

    // TEMP HACK: fix colors
    switch(c)
    {
        case RED:
        case LRED:
            c = 1;
            break;

        case BLUE:
        case LBLUE:
        case CYAN:
        case LCYAN:
            c = 2;
            break;

        default:
            c = 0;
    }

    _CallVMWrite(vm, new Falcon::CoreString(s), c);
}

void AppFalcon::_RedirectOutput(void)
{
    Falcon::Stream *sn = new MyStream(this, Falcon::stdOutputStream(), 0);
    Falcon::Stream *se = new MyStream(this, Falcon::stdOutputStream(), 1);
    // not using an actual stdErrStream is intentional

    log_setcallback(log_falcon_bridge, false, (void*)vm);

    // TODO: redirect into the same logfile that is used by the engine

    vm->stdOut(sn);
    vm->stdErr(se);
}


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
    _RedirectOutput();
    return initscript ? EmbedStringAsModule(initscript, "initscript", false, true) : true;
}

void AppFalcon::DeleteVM(void)
{
    if(vm)
    {
        log_setcallback(NULL, false, NULL);
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
