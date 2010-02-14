#ifndef OBJECTMGR_H
#define OBJECTMGR_H

#include <map>

class BaseObject;

typedef std::map<uint32, BaseObject*> ObjectMap;
typedef void (BaseObject::*cleanfunc)(void);


class ObjectMgr
{
public:
    ObjectMgr();
    ~ObjectMgr();
    uint32 Add(BaseObject*);
    BaseObject *Get(uint32 id);
    void Remove(uint32 id);
    void RemoveAll(bool del, cleanfunc f);

protected:
    uint32 _curId;
    ObjectMap _store;
};

#endif
