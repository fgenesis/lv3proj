#include "common.h"
#include "EditorEngine.h"
#include "TileLayerPanel.h"

TileLayerPanel::TileLayerPanel(EditorEngine *engine)
: _engine(engine), _mgr(NULL)
{
}

TileLayerPanel::~TileLayerPanel()
{
    for(uint32 i = 0; i < _layers.size(); ++i)
        delete _layers[i];
}

void TileLayerPanel::logic(void)
{
    if(IsCovered())
        return;

    if(isVisible())
    {
        int xo, yo;
        getAbsolutePosition(xo,yo);
        uint32 frametime = _engine->GetCurFrameTime();
        if(_mgr)
        {
            _mgr->SetRenderOffset(xo, yo);
            _mgr->Update(frametime);
        }

        for(uint32 i = 0; i < _layers.size(); ++i)
        {
            TileLayer& tl = *_layers[i];
            tl.xoffs = xo;
            tl.yoffs = yo;
            tl.Update(frametime);
        }
    }

    gcn::SelectionFramePanel::logic();
}

void TileLayerPanel::draw(gcn::Graphics *g)
{
    if(IsCovered())
        return;

    gcn::Panel::draw(g);
    if(_mgr)
        _mgr->Render();
    for(uint32 i = 0; i < _layers.size(); ++i)
        _layers[i]->Render();
    drawSelection(g);
}


/*
void EditorEngine::UpdateSelection(gcn::Widget *src)
{
    int maxX = std::min(_selOverlayRect.width / 16, (int)_selLayer->GetArraySize());
    int maxY = std::min(_selOverlayRect.height / 16, (int)_selLayer->GetArraySize());
    _selLayerBorderRect.x = _selLayer->xoffs;
    _selLayerBorderRect.y = _selLayer->yoffs;
    _selLayerBorderRect.width = maxX * 16;
    _selLayerBorderRect.height = maxY * 16;

    TileLayer *srcLayer = _GetActiveLayerForWidget(src);

    if(srcLayer)
    {
        for(int iy = 0; iy < (int)_selLayer->GetArraySize() ; iy++)
        {
            for(int ix = 0; ix < (int)_selLayer->GetArraySize(); ix++)
            {
                if(iy >= maxY || ix >= maxX)
                {
                    _selLayer->SetTile(ix, iy, NULL);
                    continue;
                }

                int tilex = (_selOverlayRect.x + (ix * 16)) / 16;
                int tiley = (_selOverlayRect.y + (iy * 16)) / 16;
                if(tilex < 0 || tiley < 0)
                    continue;

                BasicTile *tile = srcLayer->GetTile(tilex, tiley);
                _selLayer->SetTile(ix, iy, tile);
            }
        }

        // store the current selection size, it is needed to preview on the main surface where the new tiles will be put
        _selCurrentSelRect = _selOverlayRect;
    }
}
*/
