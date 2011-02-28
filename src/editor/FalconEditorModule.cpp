#include <falcon/engine.h>
#include "common.h"
#include "EditorEngine.h"
#include "DrawAreaPanel.h"
#include "BottomBarPanel.h"
#include "AppFalcon.h"


FALCON_FUNC fal_Editor_GetActiveLayerId(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(EditorEngine::GetInstance()->GetDrawPanel()->GetActiveLayerId()));
}

FALCON_FUNC fal_Editor_AddModeButton(Falcon::VMachine *vm)
{
    Falcon::Item *p0 = vm->param(0);
    if(!(p0 && p0->isString()))
    {
        throw new Falcon::ParamError( Falcon::ErrorParam( p0 ? Falcon::e_param_type : Falcon::e_inv_params ).
            extra( "S" ) );
    }
    Falcon::AutoCString cstr(vm->param(0)->asString());
    EditorEngine::GetInstance()->GetBottomPanel()->addButton(cstr.c_str());
}

FALCON_FUNC fal_Editor_SetIgnoreInput(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "B");
    EditorEngine::GetInstance()->SetIgnoreInput(vm->param(0)->isTrue());
}

FALCON_FUNC fal_Editor_EnableTileDrawing(Falcon::VMachine *vm)
{
    FALCON_REQUIRE_PARAMS_EXTRA(1, "B");

    // TODO: move this code to a better place
    EditorEngine *e = EditorEngine::GetInstance();
    DrawAreaPanel *dp = e->GetDrawPanel();

    bool enable = vm->param(0)->isTrue();
    dp->SetInputHandler(enable ? dp : NULL); // forward input to itself, i.e. normal operation, or disable forwarding
    dp->ShowSelRect(enable);
}



Falcon::Module *FalconEditorModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("EditorModule");

    Falcon::Symbol *symEditor = m->addSingleton("Editor");
    Falcon::Symbol *clsEditor = symEditor->getInstance();
    m->addClassMethod(clsEditor, "GetActiveLayerId", fal_Editor_GetActiveLayerId);
    // TODO: setting active layer will require a cleanup, and maybe switching to an event-based system instead of passing around ptrs
    m->addClassMethod(clsEditor, "AddModeButton", fal_Editor_AddModeButton);
    m->addClassMethod(clsEditor, "SetIgnoreInput", fal_Editor_SetIgnoreInput);
    m->addClassMethod(clsEditor, "EnableTileDrawing", fal_Editor_EnableTileDrawing);

    return m;
};


