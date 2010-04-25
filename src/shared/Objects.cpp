#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "LayerMgr.h"

#include "UndefUselessCrap.h"

BaseObject::BaseObject()
{
    _falObj = NULL;
    _layermgr = NULL;
    _id = 0;
    _mustdie = false;
}

void ActiveRect::Init(void)
{
    type = OBJTYPE_RECT;
    _collisionEnabled = true;
}

void ActiveRect::SetBBox(float x_, float y_, uint32 w_, uint32 h_)
{
    BaseRect::SetBBox(x_, y_, w_, h_);
    HasMoved();
}

void ActiveRect::SetPos(float x_, float y_)
{
    BaseRect::SetPos(x_, y_);
    HasMoved();
}

void ActiveRect::MoveRelative(float xr, float yr)
{
    SetPos(x + xr, y + yr);
    HasMoved();
}

// returns the side on which we hit 'other'
// (side of 'other')
// TODO: optimize this function! it should be possible to do this a lot simpler...
uint8 ActiveRect::CollisionWith(ActiveRect *other)
{
    int32 ix = int32(x);
    int32 iy = int32(y);
    int32 oix = int32(other->x);
    int32 oiy = int32(other->y);

    // self lower right corner
    int32 ax1 = ix + w - 1;
    int32 ay1 = iy + h - 1;

    // other lower right corner
    int32 bx1 = oix + other->w - 1;
    int32 by1 = oiy + other->h - 1;

    //check if bounding boxes intersect
    if((bx1 < ix) || (ax1 < oix))
        return SIDE_NONE;
    if((by1 < iy) || (ay1 < oiy))
        return SIDE_NONE;

    int32 xstart = std::max(ix,oix);
    int32 xend = std::min(ax1,bx1);

    int32 ystart = std::max(iy,oiy);
    int32 yend = std::min(ay1,by1);

    int32 width = xend - xstart;
    int32 height = yend - ystart;

    // TODO: if 'other' is completely contained in 'this', this test is always returns SIDE_LEFT
    //       just noting this here in case this has to be corrected someday, but for now it seems that its not necessary

    // check if 'this' is completely contained in 'other'
    if(ix >= oix && ax1 <= bx1 && iy >= oiy && ay1 <= by1)
    {
        // calculate both centers and guess which side we came from
        int32 xc = ix + (width / 2);
        int32 yc = iy + (height / 2);
        int32 other_xc = oix + (other->w / 2);
        int32 other_yc = oiy + (other->h / 2);
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
        if(xstart == oix)
            return SIDE_LEFT;
        else
            return SIDE_RIGHT;
    }
    else // must be top or bottom
    {
        if(ystart == oiy)
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

uint32 ActiveRect::CanMoveToDirection(uint8 d, uint32 pixels /* = 1 */)
{
    return _layermgr->CanMoveToDirection((BaseRect*)this, d, pixels);
}

float ActiveRect::GetDistanceX(ActiveRect *other)
{
    float result;
    if(this->x < other->x)
        result = other->x - (this->x + this->w);
    else
        result = this->x - (other->x + other->w);
    return fastsgncheck(result) ? 0.0f : result;
}

float ActiveRect::GetDistanceY(ActiveRect *other)
{
    float result;
    if(this->y < other->y)
        result = other->y - (this->y + this->h);
    else
        result = this->y - (other->y + other->h);
    return fastsgncheck(result) ? 0.0f : result;
}

float ActiveRect::GetDistance(ActiveRect *other)
{
    float x = GetDistanceX(other);
    float y = GetDistanceY(other);
    return sqrt(x*x + y*y);
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
    _collisionEnabled = true; // do really do collision detetion
}

void Object::SetBBox(float x_, float y_, uint32 w_, uint32 h_)
{
    ActiveRect::SetBBox(x_,y_,w_,h_);
    UpdateAnchor();
}

void Object::SetPos(float x_, float y_)
{
    ActiveRect::SetPos(x_, y_);
    UpdateAnchor();
}

// TODO: this needs correction!! maybe make this a falcon call, OR add another uint32 _standAreaSize...?
bool Object::CanFallDown(void)
{
    return _layermgr->CanFallDown(anchor, w / 2);
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

void Player::Init(void)
{
    type = OBJTYPE_PLAYER;
    _GenericInit();
}
