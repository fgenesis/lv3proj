#include "common.h"
#include "SelectionFramePanel.h"

gcn::SelectionFramePanel::SelectionFramePanel()
: Panel(0, 0), _showSelRect(false), _hilightSelRect(false), _selRectDrag(true),
_blockOffsX(0), _blockOffsY(0)
{
    addMouseListener(this);
}

gcn::SelectionFramePanel::~SelectionFramePanel()
{
}

void gcn::SelectionFramePanel::drawSelection(Graphics *g)
{
    if(!_showSelRect)
        return;

    Rectangle r = _frame;
    _fixRect(r);
    //r.x += getX();
    //r.y += getY();

    if(_hilightSelRect)
        g->setColor(Color(255,255,255,255));
    else
        g->setColor(Color(180,180,180,255));

    g->drawRectangle(r);
}

void gcn::SelectionFramePanel::draw(Graphics* g)
{
    gcn::Panel::draw(g);
    drawSelection(g);
}

void gcn::SelectionFramePanel::_initRect(uint32 x, uint32 y, bool resetWH /* = true */)
{
    _frame.x = x - ((x - _blockOffsX) % _blockW);
    _frame.y = y - ((y - _blockOffsY) % _blockH);
    if(resetWH)
    {
        _frame.width = _blockW;
        _frame.height = _blockH;
    }
}

void gcn::SelectionFramePanel::mousePressed(MouseEvent& me)
{
    _mouseBaseX = me.getX();
    _mouseBaseY = me.getY();
    _hilightSelRect = true;
    _initRect(me.getX(), me.getY());
}

void gcn::SelectionFramePanel::_alignRect(Rectangle& r)
{
    r.width += _blockW;
    r.height += _blockH;
    int32 modx = r.x % _blockW;
    int32 mody = r.y % _blockH;
    int32 modw = r.width % _blockW;
    int32 modh = r.height % _blockH;
    r.x -= modx;
    r.y -= mody;
    r.width -= modw;
    r.width -= r.x;
    r.height -= modh;
    r.height -= r.y;
    if(!r.width)
        r.width = _blockW;
    if(!r.height)
        r.height = _blockH;
}

void gcn::SelectionFramePanel::mouseMoved(MouseEvent& me)
{
    _showSelRect = true;
    _initRect(me.getX(), me.getY());
}

void gcn::SelectionFramePanel::mouseDragged(MouseEvent& me)
{
    if(!_selRectDrag)
    {
        mouseMoved(me);
        return;
    }
    _showSelRect = true;
    // select fitting position and dimension
    int32 x = me.getX();
    int32 y = me.getY();

    if(x < _mouseBaseX)
    {
        _frame.x = x < 0 ? 0 : x;
        _frame.width = _mouseBaseX;
    }
    else
    {
        _frame.x = _mouseBaseX;
        _frame.width = x;
    }

    if(y < _mouseBaseY)
    {
        _frame.y = y < 0 ? 0 : y;
        _frame.height = _mouseBaseY;
    }
    else
    {
        _frame.y = _mouseBaseY;
        _frame.height = y;
    }

    _alignRect(_frame);
}

void gcn::SelectionFramePanel::mouseReleased(MouseEvent& me)
{
    _initRect(me.getX(), me.getY(), true);
    _hilightSelRect = false;
}

void gcn::SelectionFramePanel::mouseEntered(MouseEvent& me)
{
    _showSelRect = true;
}

void gcn::SelectionFramePanel::mouseExited(MouseEvent& me)
{
    _showSelRect = false;
}

void gcn::SelectionFramePanel::_fixRect(Rectangle& r)
{
    if(r.width < 0)
    {
        r.x += r.width;
        r.width = -r.width;
    }
    if(r.height < 0)
    {
        r.y += r.height;
        r.height = -r.height;
    }
}
