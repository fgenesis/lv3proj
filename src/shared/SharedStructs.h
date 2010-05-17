#ifndef SHAREDSTRUCTS_H
#define SHAREDSTRUCTS_H
#include <limits.h>
#include "SharedDefines.h"

struct Point
{
    Point() : x(0), y(0) {}
    Point(int32 x_, int32 y_) : x(x_), y(y_) {}
    int32 x, y;

    inline bool operator==(Point& p) { return x == p.x && y == p.y; }
    inline bool operator!=(Point& p) { return !(x == p.x && y == p.y); }
    void invalidate(void) { x = y = INT_MIN; }
    bool valid(void) { return x != INT_MIN && y != INT_MIN; }
};

struct FPoint
{
    FPoint() : x(0), y(0) {}
    FPoint(float x_, float y_) : x(x_), y(y_) {}
    float x, y;

    inline bool operator==(Point& p) { return x == p.x && y == p.y; }
    inline bool operator!=(Point& p) { return !(x == p.x && y == p.y); }
    void invalidate(void) { *((int*)&x) = *((int*)&y) = 0xFF800000; } // 0xFF800000 == -1.#INF00
    bool valid(void) { return *((int*)&x) != 0xFF800000 && *((int*)&y) != 0xFF800000; }
};


// basic rectangle class, provides interfaces, but does not have any object properties
class BaseRect
{
public:

    float x, y; // we have to use floats for correct movement, so that rounding does not make fuss with movements < 1 pixel per step.
    uint32 w, h;

    // Method to calculate the second X corner
    inline int x2(void) const { return int32(x) + w; }
    // Method to calculate the second Y corner
    inline int y2(void) const { return int32(y) + h; }

    // Method to calculate the second X corner (float)
    inline float x2f(void) const { return x + float(w); }
    // Method to calculate the second Y corner (float)
    inline float y2f(void) const { return y + float(h); }

    inline BaseRect cloneRect(void)
    {
        BaseRect r;
        r.x = x;
        r.y = y;
        r.w = w;
        r.h = h;
        return r;
    }

    inline void BaseRect::SetBBox(float x_, float y_, uint32 w_, uint32 h_)
    {
        this->x = x_;
        this->y = y_;
        this->w = w_;
        this->h = h_;
    }

    inline void BaseRect::SetPos(float x_, float y_)
    {
        this->x = x_;
        this->y = y_;
    }

    inline void BaseRect::MoveRelative(float xr, float yr)
    {
        this->x += xr;
        this->y += yr;
    }

    inline bool operator==(BaseRect& other)
    {
        return int32(x) == int32(other.x) && int32(y) == int32(other.y) && w == other.w && h == other.h;
    }

    inline bool operator!=(BaseRect& other)
    {
        return int32(x) != int32(other.x) || int32(y) != int32(other.y) || w != other.w || h != other.h;
    }

    // in Objects.cpp
    uint8 CollisionWith(BaseRect *other); // returns side where the collision occurred

};

struct MovementDirectionInfo
{
    int32 xstep;
    int32 ystep;
    int32 xoffs;
    int32 yoffs;

    MovementDirectionInfo(BaseRect& rect, uint8 d)
    {
        xstep = 0;
        ystep = 0;
        xoffs = 0;
        yoffs = 0;
        if(d & DIRECTION_LEFT)
        {
            xstep = -1;
        }
        else if(d & DIRECTION_RIGHT)
        {
            xstep = 1;
            xoffs = int32(rect.w) - 1;
        }
        if(d & DIRECTION_UP)
        {
            ystep = -1;
        }
        else if(d & DIRECTION_DOWN)
        {
            ystep = 1;
            yoffs = int32(rect.h) - 1;
        }
    }
};




#endif
