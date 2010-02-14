#ifndef APP_FALCON_H
#define APP_FALCON_H

#include <falcon/engine.h>

class AppFalcon
{
public:
    AppFalcon();
    ~AppFalcon();
    void Init(void);
    void SetModulePath(char *dir) { _modulePath = dir; }
    inline Falcon::VMachine *GetVM(void) { return vm; }
    bool EmbedStringAsModule(char *str, char *modName);

    std::string ExecuteString(char *line);

protected:
    virtual void _LoadModules(void);

    Falcon::Engine::AutoInit __autoinit__;
    std::string _modulePath;
    Falcon::ModuleLoader mloader;
    Falcon::Runtime *rt;
    Falcon::VMachine *vm;
};

#define FALCON_REQUIRE_PARAMS(__p) { if(vm->paramCount() < (__p)) { throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ ));  } }
#define FALCON_REQUIRE_PARAMS_EXTRA(__p,__desc) { if(vm->paramCount() < (__p)) { throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ ).extra(__desc));  } }



#endif
