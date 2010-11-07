#include "common.h"
#include "TileWindow.h"
#include "EditorEngine.h"

TileWindow::TileWindow(EditorEngine *engine)
: gcn::Window("Tiles"), _engine(engine)
{
}

TileWindow::~TileWindow()
{
}

void TileWindow::action(const gcn::ActionEvent& ae)
{
}
