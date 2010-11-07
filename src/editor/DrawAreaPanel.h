#ifndef DRAWAREA_PANEL_H
#define DRAWAREA_PANEL_H

#include "TileLayerPanel.h"

class DrawAreaPanel : public TileLayerPanel
{
public:
    DrawAreaPanel(EditorEngine *engine);
    virtual ~DrawAreaPanel();

    virtual void logic(void);
    virtual void draw(gcn::Graphics *g);
    virtual void mouseReleased(gcn::MouseEvent& me);
    virtual void mouseMoved(gcn::MouseEvent& me);
    virtual void mousePressed(gcn::MouseEvent& me);
    virtual void mouseDragged(gcn::MouseEvent& me);

    TileLayer *GetPaintableTiles(void);
    inline TileLayerPanel *GetPreview(void) { return _preview; }
    inline uint32 GetActiveLayerId(void) { return _activeLayer; }
    void SetActiveLayer(uint32 i);

protected:
    void _DrawSelTiles(void);
    TileLayerPanel *_preview;
    uint32 _activeLayer;
};

#endif
