#include "common.h"
#include "TileWindow.h"
#include "EditorEngine.h"
#include "TileboxPanel.h"
#include "DrawAreaPanel.h"

TileWindow::TileWindow(EditorEngine *engine)
: gcn::Window("Tiles"), _engine(engine),
pBottom(4, 4),
btnPrev("  Prev  "),
btnNext("  Next  "),
btnClose("  Close  "),
laCurFolder("Current directory")
{
    setOpaque(false);
    setFrameSize(0);
    setMovable(false);
    setTitleBarHeight(0);
    btnPrev.addActionListener(this);
    btnNext.addActionListener(this);
    btnClose.addActionListener(this);
    pBottom.setBackgroundColor(gcn::Color(0x99B0FF));
    pBottom.setForegroundColor(gcn::Color(255,255,255,255));
    pBottom.SetMaxSlots(-1, 1);
    pBottom.setSize(engine->GetResX(), btnPrev.getHeight() + pBottom.GetSpacingY() * 2);
    pBottom.add(&btnPrev);
    pBottom.add(&btnNext);
    pBottom.InsertSpace(15,0);
    pBottom.add(&laCurFolder);
    // ...
    pBottom.add(&btnClose); // last button at the very right
    add(&pBottom);

    pTiles = new TileboxPanel(engine);
    pTiles->setBackgroundColor(gcn::Color(0,0,0,255));
    pTiles->SetLocked(); // do not allow accidental modification
    add(pTiles);
}

TileWindow::~TileWindow()
{
    delete pTiles;
}

void TileWindow::logic(void)
{
    // update this one only if necessary, we do not have to animate tiles that are not shown anyway
    if(isVisible())
    {
        pBottom.setY(getHeight() - pBottom.getHeight());
        uint32 freeHeight = getHeight() - pBottom.getHeight();
        pTiles->setSize(getWidth(), freeHeight);

        // the close button should stick to the very right of the lower panel
        btnClose.setX(getWidth() - btnClose.getWidth() - pBottom.GetSpacingX());
       

        gcn::Window::logic();
    }
}

void TileWindow::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();

    if(src == &btnClose)
        _engine->ToggleTileWnd(); // this can only close this window
}
