#ifndef TILELAYER_PANEL_H
#define TILELAYER_PANEL_H

#include "guichan/mouseevent.hpp"
#include "GuichanExt.h"

class EditorEngine;
class TileLayer;
class LayerMgr;

// generic tile layer panel that provides 
class TileLayerPanel : public gcn::SelectionFramePanel
{
public:
    TileLayerPanel(EditorEngine *engine);
    virtual ~TileLayerPanel();

    inline std::vector<TileLayer*>& GetTiles(void) { return _layers; }
    inline LayerMgr *GetTileMgr(void) { return _mgr; }
    inline void SetLayerMgr(LayerMgr *m) { _mgr = m; }
    
    virtual void logic(void);
    virtual void draw(gcn::Graphics *g);

protected:
    EditorEngine *_engine;
    LayerMgr *_mgr; // if not NULL, draw tiles from this mgr
    std::vector<TileLayer*> _layers; // additional layers, drawn over the mgr if present
    
};

#endif
