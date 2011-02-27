#include "common.h"
#include "EditorEngine.h"
#include "AppFalconEditor.h"

#include "FalconEditorModule.h"

AppFalconEditor::AppFalconEditor() : AppFalconGame()
{
}

void AppFalconEditor::_LoadModules(void)
{
    AppFalconGame::_LoadModules();
    vm->link(FalconEditorModule_create());
}
