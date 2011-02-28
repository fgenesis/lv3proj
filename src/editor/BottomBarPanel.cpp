#include "common.h"
#include "BottomBarPanel.h"
#include "EditorEngine.h"
#include "AppFalcon.h"

BottomBarPanel::BottomBarPanel(EditorEngine *engine)
: Panel(4, 4), _engine(engine)
{
    SetMaxSlots(-1, 1);

    // HACK: create temp button to determine height
    gcn::Button tmp("Temp");
    setSize(_engine->GetResX(), tmp.getHeight() + GetSpacingY() * 2);

    setForegroundColor(gcn::Color(200,200,200,255));
    setBackgroundColor(gcn::Color(80,0,0,100));
}

void BottomBarPanel::addButton(const char *text)
{
    gcn::Button *b = new gcn::Button(text);
    b->addActionListener(this);
    add(b);
    _buttons.push_back(b);
}


BottomBarPanel::~BottomBarPanel()
{
    clear();
}

void BottomBarPanel::clear(void)
{
    Panel::clear();
    for(uint32 i = 0; i < _buttons.size(); ++i)
        delete _buttons[i];
    _buttons.clear();
}

void BottomBarPanel::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();
    gcn::Button *btn = (gcn::Button*)src;

    Falcon::VMachine *vm = _engine->falcon->GetVM();
    Falcon::Item *item = vm->findGlobalItem("ActivateMode");
    if(item && item->isCallable())
    {
        try
        {
            Falcon::CoreString *str = new Falcon::CoreString(btn->getCaption().c_str());
            vm->pushParam(str);
            vm->callItem(*item, 1);
        }
        catch(Falcon::Error *err)
        {
            Falcon::AutoCString edesc( err->toString() );
            logerror("BottomBarPanel::action: %s", edesc.c_str());
            err->decref();
        }
    }
}
