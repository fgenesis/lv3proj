#include "common.h"
#include "BottomBarPanel.h"
#include "EditorEngine.h"

BottomBarPanel::BottomBarPanel(EditorEngine *engine)
: Panel(4, 4), _engine(engine),
btnTileBrush(" Tiles "),
btnObjBrush(" Obj ")
{
    SetMaxSlots(-1, 1);
    setSize(_engine->GetResX(), btnTileBrush.getHeight() + GetSpacingY() * 2);

    btnTileBrush.addActionListener(this);
    btnObjBrush.addActionListener(this);
    add(&btnTileBrush);
    add(&btnObjBrush);

    setForegroundColor(gcn::Color(200,200,200,255));
    setBackgroundColor(gcn::Color(80,0,0,100));
}

BottomBarPanel::~BottomBarPanel()
{
}

void BottomBarPanel::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();
    if(src == &btnTileBrush)
    {
    }
    else if(src == &btnObjBrush)
    {
    }

}
