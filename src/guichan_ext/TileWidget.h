#ifndef GCN_EXT_TILEWIDGET_H
#define GCN_EXT_TILEWIDGET_H

/*
* The TileWidget is used to display tiles on a Surface that supports selection (like a panel)
* It is NOT intended as a replacement for raw surface drawing of tiles or layering!
* This is just a proxy class which does NOT draw anything!!
* Use this to get a tile based on clicked position, for example.
*/

#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"
#include "guichan/widget.hpp"

struct BasicTile;

namespace gcn
{

    class TileWidget: public Widget
    {
    public:
        TileWidget();
        TileWidget(BasicTile *t);
        void SetTile(BasicTile *t);
        BasicTile *GetTile(void) { return _tile; }
        void draw(Graphics* graphics) {}

    protected:
        BasicTile *_tile;

    };

}

#endif

