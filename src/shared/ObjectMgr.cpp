#include "common.h"
#include "Objects.h"
#include "ObjectMgr.h"

ObjectMgr::ObjectMgr()
: _curId(0)
{
}

ObjectMgr::~ObjectMgr()
{
    RemoveAll(true, NULL);
}

void ObjectMgr::RemoveAll(bool del, cleanfunc f)
{
    for(ObjectMap::iterator it = _store.begin(); it != _store.end(); it++)
    {
        if(f)
            (it->second->*f)();
        if(del)
            delete it->second;
    }
    _store.clear();
    _curId = 0;
}

uint32 ObjectMgr::Add(BaseObject *obj)
{
    obj->_id = ++_curId;
    _store[_curId] = obj;
    return _curId;
}

BaseObject *ObjectMgr::Get(uint32 id)
{
    ObjectMap::iterator it = _store.find(id);
    if(it != _store.end())
        return it->second;

    return NULL;
}

void ObjectMgr::Remove(uint32 id)
{
    _store.erase(id);
}
