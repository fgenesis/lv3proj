#include "common.h"
#include "EditorEngine.h"
#include "TopMenus.h"

TopFileMenu::TopFileMenu(EditorEngine *e)
: MenuEngineAdapter(e)
{
    addEntry("NYI: New");
    addEntry("Open... (Ctrl+O)");
    addEntry("Save (Ctrl+S)");
    addEntry("Save as... (Ctrl+Shift+S)");
    addEntry("Exit (Alt+F4)");
}

TopFileMenu::~TopFileMenu()
{
}


void TopFileMenu::usedEntry(unsigned int which)
{
    switch(which)
    {
        case 0:
            // TOCO: implement this
            break;

        case 1:
            _engine->GetFileDlg()->Open(false, "map");
            break;

        case 2:
            _engine->_SaveCurrentMap();
            break;

        case 3:
            _engine->GetFileDlg()->Open(true, "map");
            break;

        case 4:
            _engine->SetQuit(true);
            break;

        default:
            logerror("TopFileMenu: Unhandled entry");
    }
}

TopEditMenu::TopEditMenu(EditorEngine *e)
: MenuEngineAdapter(e)
{
    addEntry("NYI: Undo");
    addEntry("NYI: Redo");
    addEntry("...");
}
TopEditMenu::~TopEditMenu()
{
}

void TopEditMenu::usedEntry(unsigned int which)
{
    PopupMenu::usedEntry(which);
}

static void setButtonX(gcn::Button *b, bool on)
{
    std::string capt = b->getCaption();
    capt[1] = (on ? 'X' : ' ');
    b->setCaption(capt);
}

TopToolsMenu::TopToolsMenu(EditorEngine *e)
: MenuEngineAdapter(e)
{
    addEntry("[X] Tilebox  (G)"); // HACK: for now, hardcode the default startup layout
    addEntry("[ ] Layers  (L)");
    addEntry("Open tile window  (T)");
}
TopToolsMenu::~TopToolsMenu()
{
}

void TopToolsMenu::usedEntry(unsigned int which)
{
    switch(which)
    {
    case 0:
        _engine->ToggleTilebox();
        break;

    case 1:
        _engine->ToggleLayerPanel();
        break;

    case 2:
        _engine->ToggleTileWnd();

    default:
        logerror("TopFileMenu: Unhandled entry");
    }
}

void TopToolsMenu::toggleTilebox(bool on)
{
    setButtonX(getButton(0), on);
}

void TopToolsMenu::toggleLayers(bool on)
{
    setButtonX(getButton(1), on);
}