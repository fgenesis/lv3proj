#ifndef FALCON_GAME_MODULE_H
#define FALCON_GAME_MODULE_H

#include <falcon/engine.h>
#include "Objects.h"

Falcon::Module *FalconGameModule_create(void);

class fal_ObjectCarrier;

// a proxy object to easily forward calls to the VM
class FalconProxyObject
{
    friend class ObjectMgr;

public:
    ~FalconProxyObject();
    void CallMethod(char *m, uint32 args = 0, ...);
    inline fal_ObjectCarrier *self(void) { return (fal_ObjectCarrier*)gclock->item().asObject(); }

    Falcon::GarbageLock *gclock;
    BaseObject *obj;
    Falcon::VMachine *vm;
};

class fal_ObjectCarrier : public Falcon::CoreObject
{
    friend class FalconProxyObject;

public:
    fal_ObjectCarrier( const Falcon::CoreClass* generator, FalconProxyObject *fobj )
    : Falcon::CoreObject( generator ), _falObj(fobj)
    {
        _obj = fobj->obj;
    }

    Falcon::CoreObject *clone() const
    {
        return NULL; // not cloneable
    }

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value );
    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const;

    inline BaseObject *GetObj(void) { return _obj; }
    inline FalconProxyObject *GetFalObj(void) { return _falObj; }

protected:
    FalconProxyObject *_falObj;
    BaseObject *_obj;

};

#endif
