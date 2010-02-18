#ifndef FALCON_GAME_MODULE_H
#define FALCON_GAME_MODULE_H

#include <falcon/engine.h>
#include "Objects.h"

Falcon::Module *FalconGameModule_create(void);

enum CoreEventTypes
{
    EVENT_TYPE_KEYBOARD = 0,
    EVENT_TYPE_JOYSTICK_BUTTON = 1,
    EVENT_TYPE_JOYSTICK_AXIS = 2,
    EVENT_TYPE_JOYSTICK_HAT = 3
};

class fal_ObjectCarrier;

// a proxy object to easily forward calls to the VM and destruction simplification
class FalconProxyObject
{
    friend class ObjectMgr;

public:
    FalconProxyObject(BaseObject *base) : obj(base) {}
    ~FalconProxyObject();
    void CallMethod(char *m, uint32 args = 0, ...);
    inline fal_ObjectCarrier *self(void) { return (fal_ObjectCarrier*)gclock->item().asObject(); }

    Falcon::GarbageLock *gclock; // to prevent deletion of the fal_ObjectCarrier when a script drops the reference
                                 // we need this, because the application keeps the objects until they are *explicitly* deleted
                                 // (via BaseObject::remove() in Falcon, or fal_BaseObject_Remove in C++)
    BaseObject *obj; // to speedup access, could be done via self()->_obj, but the self() call adds too much overhead imho
    Falcon::VMachine *vm; // the VM CallMethod invokes
};

// the fal_ObjectCarrier is the actual object stored inside the falcon VM
// it needs pointers to the FalconProxyObject and the BaseObject itself
// the FalconProxyObject will handle correct cleanup on deletion,
// and prevent that an object that was deleted in C++ but is still present in Falcon
// and beeing accessed does not cause a segfault.
class fal_ObjectCarrier : public Falcon::CoreObject
{
    friend class FalconProxyObject;

public:
    fal_ObjectCarrier( const Falcon::CoreClass* generator, FalconProxyObject *fobj )
    : Falcon::CoreObject( generator ), _falObj(fobj)
    {
        _obj = fobj->obj; // save ptr for faster access
    }
    static void init(Falcon::VMachine *vm);
    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool );
    

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
