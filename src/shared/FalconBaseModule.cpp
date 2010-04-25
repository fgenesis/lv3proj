#include <falcon/engine.h>
#include "common.h"
#include "AppFalcon.h"
#include "FalconBaseModule.h"

FALCON_FUNC fal_NullFunc(Falcon::VMachine *vm)
{
}

void forbidden_init(Falcon::VMachine *vm)
{
    throw new Falcon::AccessError( Falcon::ErrorParam( Falcon::e_noninst_cls ) );
}

FALCON_FUNC fal_include_ex( Falcon::VMachine *vm )
{
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
}

Falcon::Module *FalconBaseModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("BaseModule");

    m->addExtFunc("include_ex", fal_include_ex);

    return m;
}