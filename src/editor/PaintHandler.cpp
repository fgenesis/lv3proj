#include "common.h"
#include "EditorEngine.h"
#include "LayerPanel.h"


void EditorEngine::HandlePaintOnWidget(gcn::Widget *src, uint32 xpos, uint32 ypos, bool addSrcPos)
{
    TileLayer *target = _GetActiveLayerForWidget(src);
    if(!target)
    {
        logerror("HandlePaintOnWidget: Widget "PTRFMT" has no associated layer!");
        return;
    }
    if(addSrcPos)
    {
        xpos += src->getX();
        ypos += src->getY();
    }
    gcn::Rectangle rect = GetTargetableLayerTiles(xpos, ypos,
        _selLayerBorderRect.width, _selLayerBorderRect.height,
        _selLayer->GetArraySize(), _selLayer->GetArraySize(), target);
    for(uint32 y = 0; y < uint32(rect.height); y++)
    {
        for(uint32 x = 0; x < uint32(rect.width); x++)
        {
            BasicTile *tile = _selLayer->GetTile(x,y);
            target->SetTile(x + rect.x, y + rect.y, tile);
        }
    }
    panLayers->UpdateStats(target);
}
