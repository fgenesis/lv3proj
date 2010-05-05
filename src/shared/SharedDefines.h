#ifndef SHAREDDEFINES_H
#define SHAREDDEFINES_H

enum Direction
{
    DIRECTION_NONE   = 0x00,
    DIRECTION_UP     = 0x01,
    DIRECTION_DOWN   = 0x02,
    DIRECTION_LEFT   = 0x04,
    DIRECTION_RIGHT  = 0x08,

    DIRECTION_UPLEFT = DIRECTION_UP | DIRECTION_LEFT,
    DIRECTION_UPRIGHT = DIRECTION_UP | DIRECTION_RIGHT,
    DIRECTION_DOWNLEFT = DIRECTION_DOWN | DIRECTION_LEFT,
    DIRECTION_DOWNRIGHT = DIRECTION_DOWN | DIRECTION_RIGHT,
};

// side and direction values beeing the same is intentional!
enum Side
{
    SIDE_NONE = DIRECTION_NONE,
    SIDE_TOP = DIRECTION_UP,
    SIDE_BOTTOM = DIRECTION_DOWN,
    SIDE_LEFT = DIRECTION_LEFT,
    SIDE_RIGHT = DIRECTION_RIGHT,

    SIDE_TOPLEFT = SIDE_TOP | SIDE_LEFT,
    SIDE_TOPRIGHT = SIDE_TOP | SIDE_RIGHT,
    SIDE_BOTTOMLEFT = SIDE_BOTTOM | SIDE_LEFT,
    SIDE_BOTTOMRIGHT = SIDE_BOTTOM | SIDE_RIGHT,

    SIDE_ALL = SIDE_TOPLEFT | SIDE_BOTTOMRIGHT,

    SIDE_FLAG_SOLID = 0x80 // special, set by the physics mgr for a simulated OnTouch() call
};

// TODO: this can be solved faster with some evil shifting like ((s << 1) | (s >> 1)) & ...
// but go the safe way for now
inline uint8 InvertSide(uint8 s)
{
    uint32 n = s & ~SIDE_ALL;

    if(s & SIDE_TOP)         n |= SIDE_BOTTOM;
    else if(s & SIDE_BOTTOM) n |= SIDE_TOP;

    if(s & SIDE_LEFT)        n |= SIDE_RIGHT;
    else if(s & SIDE_RIGHT)  n |= SIDE_LEFT;

    return n;
}

enum CoreEventTypes
{
    EVENT_TYPE_KEYBOARD = 0,
    EVENT_TYPE_JOYSTICK_BUTTON = 1,
    EVENT_TYPE_JOYSTICK_AXIS = 2,
    EVENT_TYPE_JOYSTICK_HAT = 3
};

#endif
