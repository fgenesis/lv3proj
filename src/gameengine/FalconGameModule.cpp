#include <falcon/engine.h>
#include "common.h"
#include "GameEngine.h"
#include "PhysicsSystem.h"
#include "AppFalcon.h"
#include "FalconBaseModule.h"


FALCON_FUNC fal_Physics_SetGravity(Falcon::VMachine *vm)
{
    Falcon::Item *p0 = vm->param(0);
    Vector2df& grav = GameEngine::GetInstance()->physmgr->envPhys.gravity;

    switch(vm->paramCount())
    {
        case 1:
        {
            if(p0->isObject())
            {
                Falcon::Item *vi = vm->findWKI("Vector");
                DEBUG(ASSERT(vi));
                Falcon::CoreClass *vcls = vi->asClass();
                Falcon::CoreObject *vo = p0->asObject();
                if(vo->generator() == vcls)
                {
                    fal_Vector2d *fv = Falcon::dyncast<fal_Vector2d*>(vo);
                    grav = fv->vec();
                    return;
                }
            }
            else
            {
                 grav = Vector2df(0, float(p0->forceNumeric()));
                 return;
            }
        }
        case 2:
            grav = Vector2df(float(p0->forceNumeric()), float(vm->param(1)->forceNumeric()));
            return;
    }

    throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
        .extra("N | N,N | Vector2d") );
}

FALCON_FUNC fal_Physics_GetGravity(Falcon::VMachine *vm)
{
    // TODO: use externally stored carrier
    Falcon::Item *vi = vm->findWKI("Vector");
    DEBUG(ASSERT(vi));
    Falcon::CoreClass *vcls = vi->asClass();
    vm->retval(GameEngine::GetInstance()->physmgr->envPhys.gravity.y);
    vm->retval(new fal_Vector2d(vcls, &GameEngine::GetInstance()->physmgr->envPhys.gravity));
}


Falcon::Module *FalconGameModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("GameModule");

    Falcon::Symbol *symPhysics = m->addSingleton("Physics");
    Falcon::Symbol *clsPhysics = symPhysics->getInstance();
    m->addClassMethod(clsPhysics, "SetGravity", fal_Physics_SetGravity);
    m->addClassMethod(clsPhysics, "GetGravity", fal_Physics_GetGravity);

    return m;
};


