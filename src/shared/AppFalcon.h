#ifndef APP_FALCON_H
#define APP_FALCON_H

#include <falcon/engine.h>

class Engine;

class AppFalcon
{
public:
    AppFalcon();
    ~AppFalcon();
    bool Init(char *initscript = NULL);
    void DeleteVM(void);
    void SetModulePath(char *dir) { _modulePath = dir; }
    inline Falcon::VMachine *GetVM(void) { return vm; }
    bool EmbedStringAsModule(char *str, char *modName, bool throw_ = false, bool launch = false);

protected:
    void _RedirectOutput(void);
    virtual void _LoadModules(void);
    void _LinkModule(Falcon::Module *m);

    std::string _modulePath;
    Falcon::ModuleLoader mloader;
    Falcon::VMachine *vm;
};

#define FALCON_REQUIRE_PARAMS(__p) { if(vm->paramCount() < (__p)) { throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ ));  } }
#define FALCON_REQUIRE_PARAMS_EXTRA(__p,__desc) { if(vm->paramCount() < (__p)) { throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ ).extra(__desc));  } }


inline Falcon::CoreClass *LookupFalconClass(Falcon::VMachine *vm, const char *name)
{
    Falcon::Item *itm = vm->findWKI(name);
    ASSERT(itm != NULL);
    return itm->asClass();
}


#endif
