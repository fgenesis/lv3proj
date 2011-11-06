#ifndef FALCON_OBJECT_MODULE_H
#define FALCON_OBJECT_MODULE_H

#include "FalconHelpers.h"


class BaseObject;
class TileLayer;
class BasicTile;

Falcon::Module *FalconObjectModule_create(void);


class fal_ObjectCarrier;
class fal_PhysProps;

// a proxy object to easily forward calls to the VM and destruction simplification
class FalconProxyObject
{
    friend class ObjectMgr;

public:
    FalconProxyObject(BaseObject *base) : obj(base) {}
    ~FalconProxyObject();

    Falcon::Item *CallMethod(const char *m);
    Falcon::Item *CallMethod(const char *m, const Falcon::Item& a);
    Falcon::Item *CallMethod(const char *m, const Falcon::Item& a, const Falcon::Item& b);
    Falcon::Item *CallMethod(const char *m, const Falcon::Item& a, const Falcon::Item& b, const Falcon::Item& c);
    // extend if methods with more params are necessary

    inline fal_ObjectCarrier *self(void) { return (fal_ObjectCarrier*)gclock->item().asObject(); }

    Falcon::GarbageLock *gclock; // to prevent deletion of the fal_ObjectCarrier when a script drops the reference
    // we need this, because the application keeps the objects until they are *explicitly* deleted
    // (via BaseObject::remove() in Falcon, or fal_BaseObject_Remove in C++)
    BaseObject *obj; // to speedup access, could be done via self()->_obj, but the self() call adds too much overhead imho
    Falcon::VMachine *vm; // the VM CallMethod invokes
    const Falcon::CoreClass *coreCls; // the internal class the object belongs to

private:
    bool _PrepareMethod(const char *m, Falcon::Item& mth); // return true if method was found, changes mth param
    Falcon::Item *_CallReadyMethod(const char *mthname, const Falcon::Item& mth, uint32 args); // mthname here only used in case of error
};

// the fal_ObjectCarrier is the actual object stored inside the falcon VM
// it needs pointers to the FalconProxyObject and the BaseObject itself
// the FalconProxyObject will handle correct cleanup on deletion,
// and prevent that an object that was deleted in C++ but is still present in Falcon
// and beeing accessed does not cause a segfault.

// TODO: use the new carrier model from FalconHelpers.h
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
    virtual void gcMark(Falcon::uint32 g);

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

// capsules a std::container<T> for use in falcon.
// V must be some container<T>
// (unused right now, i think... kept for later)
/*
template <typename T, typename V = std::vector<T> > class fal_VectorAdapter : public Falcon::CoreObject
{
public:
    fal_VectorAdapter( const Falcon::CoreClass* generator, V *blob)
        : CoreObject(generator), v(blob), 
    {
    }

    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool )
    {
        return new fal_VectorAdapter<T,V>(cls, NULL);
    }

    static void init(Falcon::VMachine *vm) {}

    virtual Falcon::CoreObject *clone() const
    {
        fal_VectorAdapter *va = new fal_VectorAdapter<T,V>(generator(), NULL);
        va->v->resize(v->size());
        std::copy(v->begin(), v->end(), va->v->begin()); // for type safety, use this instead of memcpy()
        return va;
    }

    virtual bool finalize(void) { return true; }

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value )
    {
        return false;
    }

    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
    {
        return defaultProperty(prop, ret);
    }

    void Set(const Falcon::Item& itm)
    {
        switch(itm.type())
        {
            case FLC_ITEM_ARRAY:
            {
                Falcon::CoreArray *a = itm.asArray();
                uint32 m = std::min<uint32>(v->size(), a->length());
                for(uint32 i = 0; i < m; ++i)
                    (*v)[i] = ConvertFalconItem<T>(a->at(i));
                return;
            }

            Falcon::CoreObject *obj = itm.asObjectSafe();
            if(obj && obj->generator() == generator()) // this respects template specialization
            {
                fal_VectorAdapter<T,V> *va = Falcon::dyncast<fal_VectorAdapter<T,V>*>(obj);
                uint32 m = std::min<uint32>(v->size(), va->v->size());
                for(uint32 i = 0; i < m; ++i)
                    (*v)[i] = (*va->v)[i];
            }

            throw new Falcon::ParamError(Falcon::ErrorParam( Falcon::e_inv_params, __LINE__ )
                .extra("Array|VectorAdapter") );

        }
    }

    V *v; // ptr to container type
};
*/



#endif
