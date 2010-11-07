#include "common.h"
#include "TileboxPanel.h"
#include "EditorEngine.h"
#include "TileboxPanel.h"
#include "DrawAreaPanel.h"

TileboxPanel::TileboxPanel(EditorEngine *engine)
: TileLayerPanel(engine), _locked(false)
{
}

TileboxPanel::~TileboxPanel()
{
}

void TileboxPanel::mousePressed(gcn::MouseEvent& me)
{
    // right click puts currently selected tiles into the tilebox at the chosen position
    if(me.getButton() == gcn::MouseEvent::RIGHT)
    {
        _DrawSelTiles();
    }
    else // left click does nothing specific, except selecting tiles, but this is handled in mouseReleased().
    {
        TileLayerPanel::mousePressed(me);
    }
}

void TileboxPanel::_DrawSelTiles(void)
{
    if(_locked)
        return;

    DrawAreaPanel *drawp = _engine->GetDrawPanel();
    uint32 w = drawp->GetSelBlocksW();
    uint32 h = drawp->GetSelBlocksH();
    drawp->GetPaintableTiles()->CopyTo(0, 0, GetTileLayer(), GetSelBlocksX(), GetSelBlocksY(), w, h);
}

void TileboxPanel::mouseReleased(gcn::MouseEvent& me)
{
    // select tiles with left click
    if(me.getButton() == gcn::MouseEvent::LEFT)
    {
        // adjust size of frame on draw panel
        DrawAreaPanel *drawp = _engine->GetDrawPanel();
        gcn::Rectangle& dframe = drawp->GetFrame();
        dframe.width = _frame.width;
        dframe.height = _frame.height;
        TileLayer *ptiles = drawp->GetPaintableTiles();

        // enlarge tile storage as necessary
        uint32 tilesw = GetSelBlocksW();
        uint32 tilesh = GetSelBlocksH();
        uint32 reqdim = std::max<uint32>(tilesw, tilesh);
        if(ptiles->GetArraySize() < reqdim)
            ptiles->Resize(reqdim);

        // copy tiles into the storage
        GetTileLayer()->CopyTo(GetSelBlocksX(), GetSelBlocksY(), ptiles, 0, 0, tilesw, tilesh);
    }

    gcn::SelectionFramePanel::mouseReleased(me);

}

void TileboxPanel::mouseEntered(gcn::MouseEvent& me)
{
    _engine->GetDrawPanel()->ShowSelRect(false);
    gcn::SelectionFramePanel::mouseEntered(me);
}

TileLayer * TileboxPanel::GetTileLayer(void)
{
    DEBUG(ASSERT(_layers.size() == 1));
    return _layers[0];
}

void TileboxPanel::mouseDragged(gcn::MouseEvent& me)
{
    if(me.getButton() == gcn::MouseEvent::RIGHT)
    {
        // temp. disable dragging, so that the selection box will not enlarge (that would look confusing!)
        bool drag = _selRectDrag;
        _selRectDrag = false;
        TileLayerPanel::mouseDragged(me);
        _selRectDrag = drag;

        // write tiles to position dragged to
        _DrawSelTiles();
    }
    else
    {
        TileLayerPanel::mouseDragged(me);
    }
}