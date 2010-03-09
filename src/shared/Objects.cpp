#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"

#include "UndefUselessCrap.h"

BaseObject::BaseObject()
{
    _falObj = NULL;
    _id = 0;
}

void ActiveRect::Init(void)
{
    type = OBJTYPE_RECT;
}

void ActiveRect::SetBBox(int32 x_, int32 y_, uint32 w_, uint32 h_)
{
    this->x = x_;
    this->y = y_;
    this->w = w_;
    this->h = h_;
}

void ActiveRect::SetPos(int32 x_, int32 y_)
{
    this->x = x_;
    this->y = y_;
}

// returns the side on which we hit 'other'
// (side of 'other')
// TODO: optimize this function! it should be possible to do this a lot simpler...
uint8 ActiveRect::CollisionWith(ActiveRect *other)
{

    // self lower right corner
    int32 ax1 = x + w - 1;
    int32 ay1 = y + h - 1;

    // other lower right corner
    int32 bx1 = other->x + other->w - 1;
    int32 by1 = other->y + other->h - 1;

    //check if bounding boxes intersect
    if((bx1 < x) || (ax1 < other->x))
        return SIDE_NONE;
    if((by1 < y) || (ay1 < other->y))
        return SIDE_NONE;

    int32 xstart = std::max(x,other->x);
    int32 xend = std::min(ax1,bx1);

    int32 ystart = std::max(y,other->y);
    int32 yend = std::min(ay1,by1);

    int32 width = xend - xstart;
    int32 height = yend - ystart;

    // TODO: if 'other' is completely contained in 'this', this test is always returns SIDE_LEFT
    //       just noting this here in case this has to be corrected someday, but for now it seems that its not necessary

    // check if 'this' is completely contained in 'other'
    if(x >= other->x && ax1 <= bx1 && y >= other->y && ay1 <= by1)
    {
        // calculate both centers and guess which side we came from
        int32 xc = x + (width / 2);
        int32 yc = y + (height / 2);
        int32 other_xc = other->x + (other->w / 2);
        int32 other_yc = other->y + (other->h / 2);
        int32 xcd = abs(xc - other_xc);
        int32 ycd = abs(yc - other_yc);
        if(ycd >= xcd) // height diff is greater than width diff, so we came from top or bottom
        {
            if(yc < other_yc)
                return SIDE_TOP;
            else
                return SIDE_BOTTOM;
        }
        else // width diff is greater, so we came from left or right
        {
            if(xc < other_xc)
                return SIDE_LEFT;
            else
                return SIDE_RIGHT;
        }
    }

    if(height >= width) // must be left or right
    {
        if(xstart == other->x)
            return SIDE_LEFT;
        else
            return SIDE_RIGHT;
    }
    else // must be top or bottom
    {
        if(ystart == other->y)
            return SIDE_TOP;
        else
            return SIDE_BOTTOM;
    }

    NOT_REACHED_LINE;
}

void ActiveRect::AlignToSideOf(ActiveRect *other, uint8 side)
{
    SetMoved(true);
    // TODO: ...
}

void Object::Init(void)
{
    type = OBJTYPE_OBJECT;
    _GenericInit();
}

void Object::_GenericInit(void)
{
    memset(&phys, 0, sizeof(PhysProps)); // TODO: apply some useful default values
    _physicsAffected = false;
    _layerId = LAYER_DEFAULT_SPRITES;
    _oldLayerId = 0; // update in first update cycle anyways
    _gfx = NULL;
    _moved = true; // do collision detection on spawn
}

void Item::Init(void)
{
    type = OBJTYPE_ITEM;
    _GenericInit();
}

void Unit::Init(void)
{
    type = OBJTYPE_UNIT;
    _GenericInit();
}

void Unit::SetBBox(int32 x_, int32 y_, uint32 w_, uint32 h_)
{
    Object::SetBBox(x_,y_,w_,h_);
    UpdateAnchor();
}

void Unit::SetPos(int32 x_, int32 y_)
{
    Object::SetPos(x_, y_);
    UpdateAnchor();
}


void Player::Init(void)
{
    type = OBJTYPE_PLAYER;
    _GenericInit();
}
