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

    gcn::Panel::draw(g); // we call the normal panel draw routine here, because the selection frame is treated specially below
    if(_mgr)
        _mgr->Render();
    for(uint32 i = 0; i < _layers.size(); ++i)
        _layers[i]->Render();
    drawSelection(g); // last; it would be overdrawn otherwise
}
