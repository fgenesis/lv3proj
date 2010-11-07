#ifndef TILEBOXPANEL_H
#define TILEBOXPANEL_H

#include "SelectionFramePanel.h"
#include "TileLayerPanel.h"


class TileboxPanel : public TileLayerPanel
{
public:
    TileboxPanel(EditorEngine *engine);
    virtual ~TileboxPanel();

    virtual void mousePressed(gcn::MouseEvent& me);
    virtual void mouseReleased(gcn::MouseEvent& me);
    virtual void mouseEntered(gcn::MouseEvent& me);
    virtual void mouseDragged(gcn::MouseEvent& me);

    inline void SetLocked(bool b = true) { _locked = b; }
    inline bool IsLocked(void) { return _locked; }

    TileLayer *GetTileLayer(void);

protected:
    void _DrawSelTiles(void);
    
    bool _locked;


};

#endif
