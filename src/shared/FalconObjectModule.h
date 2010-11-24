#ifndef FALCON_OBJECT_MODULE_H
#define FALCON_OBJECT_MODULE_H


class BaseObject;
class TileLayer;
class BasicTile;

Falcon::Module *FalconObjectModule_create(void);
void FalconObjectModule_SetEnginePtr(Engine *eng);


class fal_ObjectCarrier;

// a proxy object to easily forward calls to the VM and destruction simplification
class FalconProxyObject
{
    friend class ObjectMgr;

public:
    FalconProxyObject(BaseObject *base) : obj(base) {}
    ~FalconProxyObject();
    Falcon::Item *CallMethod(char *m, uint32 args = 0, ...);
    inline fal_ObjectCarrier *self(void) { return (fal_ObjectCarrier*)gclock->item().asObject(); }

    Falcon::GarbageLock *gclock; // to prevent deletion of the fal_ObjectCarrier when a script drops the reference
    // we need this, because the application keeps the objects until they are *explicitly* deleted
    // (via BaseObject::remove() in Falcon, or fal_BaseObject_Remove in C++)
    BaseObject *obj; // to speedup access, could be done via self()->_obj, but the self() call adds too much overhead imho
    Falcon::VMachine *vm; // the VM CallMethod invokes
    const Falcon::CoreClass *coreCls; // the internal class the object belongs to
};

// the fal_ObjectCarrier is the actual object stored inside the falcon VM
// it needs pointers to the FalconProxyObject and the BaseObject itself
// the FalconProxyObject will handle correct cleanup on deletion,
// and prevent that an object that was deleted in C++ but is still present in Falcon
// and beeing accessed does not cause a segfault.
class fal_ObjectCarrier : public Falcon::FalconObject
{
    friend class FalconProxyObject;

public:
    fal_ObjectCarrier( const Falcon::CoreClass* generator, FalconProxyObject *fobj )
        : Falcon::FalconObject( generator ), _falObj(fobj)
    {
        _obj = fobj->obj; // save ptr for faster access
        fobj->coreCls = generator; // store internal class
    }
    static void init(Falcon::VMachine *vm);
    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool );


    Falcon::FalconObject *clone() const
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

class fal_Tile : public Falcon::CoreObject
{
public:
    fal_Tile( const Falcon::CoreClass* generator, BasicTile *obj );

    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool )
    {
        return new fal_Tile(cls, NULL);
    }

    static void init(Falcon::VMachine *vm);

    Falcon::CoreObject *clone() const
    {
        return NULL; // not cloneable
    }

    virtual bool finalize(void);

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value );
    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const;

    inline BasicTile *GetTile(void) { return _tile; }

private:
    BasicTile *_tile;
};


#endif
