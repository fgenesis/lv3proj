#include "guichan/exception.hpp"
#include "Panel.h"

gcn::Panel::Panel(int initialSpaceX, int initialSpaceY)
: _coveredBy(NULL)
{
    _slotsx = -1;
    _slotsy = -1;
    _maxheight = 0;
    _spacingX = initialSpaceX;
    _spacingY = initialSpaceY;
    setFocusable(false);
    clear();
}

void gcn::Panel::clear(void)
{
    _usedx = 0;
    _usedy = 0;
    _nextx = _spacingX;
    _nexty = _spacingY;
    gcn::Container::clear();
}

void gcn::Panel::SetSpacing(int x, int y)
{
    _spacingX = x;
    _spacingY = y;
}

void gcn::Panel::SetMaxSlots(int x, int y)
{
    _slotsx = x;
    _slotsy = y;
}

void gcn::Panel::draw(Graphics* graphics)
{
    if(IsCovered())
        return;

    // draw self, background
    if(getBackgroundColor().a) // draw background only if not completely transparent
    {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(getChildrenArea());
    }
    if(getForegroundColor().a) // draw foreground only if not completely transparent
    {
        graphics->setColor(getForegroundColor());
        graphics->drawRectangle(getChildrenArea());
    }

    drawChildren(graphics);
}

void gcn::Panel::add(gcn::Widget *widget)
{
    // width of current widget exceeds remaining space, go down 1 row
    int dimx = _nextx + _spacingX + widget->getDimension().width;
    if(dimx > getChildrenArea().width || (_slotsx > 0 && _usedx >= _slotsx))
    {
        _usedx = 0;
        _nextx = _spacingX;
        _usedy++;
        _nexty += (_maxheight + _spacingY);
        _maxheight = 0;
    }

    // height of current widget exceeds remaining space, whoops
    int dimy = _nexty + _spacingY + widget->getDimension().height;
    if(dimy > getChildrenArea().height && _slotsy >= 0 && _usedy >= _slotsy)
    {
        throw GCN_EXCEPTION("height exceeded, can't add more widgets");
    }

    widget->setPosition(_nextx, _nexty);
    Container::add(widget);
    _maxheight = std::max(_maxheight, widget->getDimension().height);
    
    _nextx += (_spacingX + widget->getDimension().width);
    _usedx++;
}



