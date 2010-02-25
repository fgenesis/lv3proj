#include "common.h"
#include "Objects.h"

BaseObject::BaseObject()
{
    _falObj = NULL;
    _id = 0;
}

void ActiveRect::Init(void)
{
    type = OBJTYPE_RECT;
}

void Object::Init(void)
{
    type = OBJTYPE_OBJECT;
}

void Item::Init(void)
{
    type = OBJTYPE_ITEM;
}

void Unit::Init(void)
{
    type = OBJTYPE_UNIT;
}

void Player::Init(void)
{
    type = OBJTYPE_PLAYER;
}