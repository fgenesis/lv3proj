#ifndef GCN_EXT_SELECTIONFRAMEPANEL_H
#define GCN_EXT_SELECTIONFRAMEPANEL_H

#include "guichan/mouselistener.hpp"
#include "Panel.h"
#include "InputHandlerBase.h"

namespace gcn
{

class SelectionFramePanel : public Panel, public InputHandlerBase
{
public:
    SelectionFramePanel();
    ~SelectionFramePanel();

    virtual void draw(Graphics* g);
    virtual void drawSelection(Graphics *g);

    virtual void mousePressed(MouseEvent& me);
    virtual void mouseDragged(MouseEvent& me);
    virtual void mouseMoved(MouseEvent& me);
    virtual void mouseReleased(MouseEvent& me);
    virtual void mouseExited(MouseEvent& me);
    virtual void mouseEntered(MouseEvent& me);

    inline void SetBlockSize(uint32 w, uint32 h) { _blockW = w; _blockH = h; _frame.width = w, _frame.height = h; }
    inline uint32 GetBlockW(void) { return _blockW; }
    inline uint32 GetBlockH(void) { return _blockH; }
    inline void ShowSelRect(bool b) { _showSelRect = b; }
    inline bool ShowSelRect(void) { return _showSelRect; }
    inline void SetDraggable(bool b) { _selRectDrag = b; }
    inline bool IsDraggable(void) { return _selRectDrag; }
    inline Rectangle& GetFrame(void) { return _frame; }

    inline uint32 GetSelBlocksX(void) { return _frame.x / _blockW; } // x position
    inline uint32 GetSelBlocksY(void) { return _frame.y / _blockH; } // y position
    inline uint32 GetSelBlocksW(void) { return _frame.width / _blockW; }
    inline uint32 GetSelBlocksH(void) { return _frame.height / _blockH; }

protected:
    void _initRect(uint32 x, uint32 y, bool resetWH = false);
    void _fixRect(Rectangle& r);
    void _alignRect(Rectangle& r);

    InputHandlerBase _rootHandler;

    int32 _blockW, _blockH;
    int32 _blockOffsX, _blockOffsY;
    bool _hilightSelRect;
    bool _showSelRect;
    bool _selRectDrag;
    Rectangle _frame;
    int32 _mouseBaseX; // position of click, if dragged
    int32 _mouseBaseY;
};

}


#endif
