#ifndef COLLISION_UTIL_H
#define COLLISION_UTIL_H

#include "Vector2d.h"


template <typename T> bool CastBresenhamLine(int32 x0, int32 y0, int32 x1, int32 y1,
                                             Vector2di *last, Vector2di *pos, T callback)
{
    int32 dx = abs(x1-x0);
    int32 dy = abs(y1-y0);
    int32 sx = x0 < x1 ? 1 : -1;
    int32 sy = y0 < y1 ? 1 : -1;
    int32 err = dx - dy;
    int32 e2;
    int32 lastx = x0, lasty = y0;

    while(true)
    {
        if(callback(x0, y0))
        {
            if(last)
                *last = Vector2di(lastx, lasty);
            if(pos)
                *pos = Vector2di(x0, y0);
            return true;
        }
        lastx = x0;
        lasty = y0;
        if(x0 == x1 && y0 == y1)
            break;

        e2 = err * 2;
        if(e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if(e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }

    return false;
}

#endif
