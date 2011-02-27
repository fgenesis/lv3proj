#include <falcon/engine.h>
#include "common.h"
#include "EditorEngine.h"
#include "DrawAreaPanel.h"
#include "AppFalcon.h"


FALCON_FUNC fal_Editor_GetActiveLayerId(Falcon::VMachine *vm)
{
    vm->retval(Falcon::int32(EditorEngine::GetInstance()->GetDrawPanel()->GetActiveLayerId()));
}

Falcon::Module *FalconEditorModule_create(void)
{
    Falcon::Module *m = new Falcon::Module;
    m->name("EditorModule");

    Falcon::Symbol *symEditor = m->addSingleton("Editor");
    Falcon::Symbol *clsEditor = symEditor->getInstance();
    m->addClassMethod(clsEditor, "GetActiveLayerId", fal_Editor_GetActiveLayerId);
    // TODO: setting active layer will require a cleanup, and maybe switching to an event-based system instead of passing around ptrs

    return m;
};


