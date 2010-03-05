#ifndef SHAREDSTRUCTS_H
#define SHAREDSTRUCTS_H

struct Point
{
    Point() : x(0), y(0) {}
    Point(int32 x_, int32 y_) : x(x_), y(y_) {}
    uint32 x;
    uint32 y;
};

#endif
