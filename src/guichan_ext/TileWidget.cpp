#include "common.h"
#include "Tile.h"
#include "TileWidget.h"

gcn::TileWidget::TileWidget()
{
    _tile = NULL;
    setHeight(16);
    setWidth(16);
}

gcn::TileWidget::TileWidget(BasicTile *t)
{
    _tile = t;
    setHeight(16);
    setWidth(16);
}

void gcn::TileWidget::SetTile(BasicTile *t)
{
    _tile = t;
}

