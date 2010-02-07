#ifndef GCN_EXT_LABEL_H
#define GCN_EXT_LABEL_H

#include <string>

#include "guichan/graphics.hpp"
#include "guichan/platform.hpp"
#include "guichan/widgets/container.hpp"

namespace gcn
{

    class Panel: public Container
    {
    public:
        Panel(int initialSpaceX, int initialSpaceY);

        // overloaded
        virtual void draw(Graphics* graphics);
        virtual void add(Widget* widget);

        virtual void SetSpacing(int x, int y);
        virtual int GetSpacingX(void) { return _spacingX; }
        virtual int GetSpacingY(void) { return _spacingY; }
        virtual int GetNextX(void) { return _nextx; }
        virtual void SetMaxSlots(int x, int y);
        virtual void InsertSpace(int x, int y) { _nextx += x; _nexty += y; }


    protected:
        int _slotsx; // max slot count
        int _slotsy;
        int _usedx; // used slots
        int _usedy;
        int _nextx; // next draw pos. relative to children rect!
        int _nexty;
        int _maxheight; // max used height in current row

        int _spacingX;
        int _spacingY;

    };

}


#endif
