#include <stack>

#include "common.h"
#include "SharedDefines.h"
#include "Objects.h"
#include "Engine.h"

#include "UndefUselessCrap.h"

BaseObject::BaseObject()
{
    _falObj = NULL;
    _layermgr = NULL;
    _id = 0;
}

BaseObject::~BaseObject()
{
    DEBUG(logdebug("~BaseObject "PTRFMT, this));
}

// single-directional non-recursive DFS graph search
void BaseObject::GetAttached(std::set<BaseObject*>& found) const
{
    std::stack<const BaseObject*> pending;
    pending.push(this);

    const BaseObject *obj;
    do 
    {
        obj = pending.top();
        pending.pop();
        found.insert(const_cast<BaseObject*>(obj));

        for(std::set<BaseObject*>::const_iterator it = obj->_children.begin(); it != obj->_children.end(); ++it)
        {
            if(found.find(*it) == found.end())
                pending.push(*it);
        }
    }
    while (pending.size());
}

void ActiveRect::Init(void)
{
    type = OBJTYPE_RECT;
    _collisionMask = -1; // all flags set
    _update = false;
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

void ActiveRect::Move(float xr, float yr)
{
    SetPos(x + xr, y + yr);
    HasMoved();
}

float ActiveRect::GetDistanceX(ActiveRect *other) const
{
    float result;
    if(this->x < other->x)
        result = other->x - (this->x + this->w);
    else
        result = this->x - (other->x + other->w);
    return result < 0.0f ? 0.0f : result;
}

float ActiveRect::GetDistanceY(ActiveRect *other) const
{
    float result;
    if(this->y < other->y)
        result = other->y - (this->y + this->h);
    else
        result = this->y - (other->y + other->h);
    return result < 0.0f ? 0.0f : result;
}

float ActiveRect::GetDistance(ActiveRect *other) const
{
    float x = GetDistanceX(other);
    float y = GetDistanceY(other);
    return sqrt(x*x + y*y);
}

bool ActiveRect::CastRay(const Vector2di& dir, Vector2di& lastpos, Vector2di& collpos, LayerCollisionFlag lcf /* = LCF_ALL*/) const
{
    return Engine::GetInstance()->_GetLayerMgr()->CastRaysFromRect(*this, dir, lastpos, collpos, lcf);
}


void Object::Init(void)
{
    type = OBJTYPE_OBJECT;
    _GenericInit();
}

Object::~Object(void)
{
    SetSprite(NULL); // this will handle refcounting
}

void Object::_GenericInit(void)
{
    _physicsAffected = false;
    _oldLayerId = _layerId = LAYER_MAX / 2; // place on middle layer by default
    _gfx = NULL;
    _moved = true; // do collision detection on spawn
    _collisionMask = -1; // enable all flags
    _oldLayerRect.x = 0;
    _oldLayerRect.y = 0;
    _oldLayerRect.w = 0;
    _oldLayerRect.h = 0;
    _update = true;
    _visible = true;
    _ownLCF = LCF_NONE;
    _blockedByLCF = LCF_ALL; // FIXME: which default is sane?
}

void Object::SetSprite(BasicTile *tile)
{
    if(tile == _gfx)
        return;
    if(tile)
        tile->ref++;
    if(_gfx)
        _gfx->ref--;
    _gfx = tile;
}

Vector2df Object::GetTotalSpeed(void) const
{
    Vector2df v;
    for(size_t i = 0; i < phys.size(); ++i)
        v += phys[i].speed;
    return v;
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
