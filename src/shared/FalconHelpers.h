#ifndef FALCON_HELPERS_H
#define FALCON_HELPERS_H

#include <falcon/garbagelock.h>
#include <falcon/coreobject.h>
#include <falcon/cacheobject.h>

template <typename T> inline T ConvertFalconItem(const Falcon::Item& itm)
{
    ASSERT(0 && "ConvertFalconItem<T>() -- nonspecialized template called, implement me!");
    NOT_REACHED_LINE;
    return T();
}

template <> inline float ConvertFalconItem<float>(const Falcon::Item& itm)
{
    return (float)itm.forceNumeric();
}

template <> inline double ConvertFalconItem<double>(const Falcon::Item& itm)
{
    return itm.forceNumeric();
}

template <> inline uint32 ConvertFalconItem<uint32>(const Falcon::Item& itm)
{
    return (uint32)itm.forceInteger();
}

template <> inline int32 ConvertFalconItem<int32>(const Falcon::Item& itm)
{
    return (int32)itm.forceInteger();
}

template <> inline uint64 ConvertFalconItem<uint64>(const Falcon::Item& itm)
{
    return (uint64)itm.forceInteger();
}

template <> inline int64 ConvertFalconItem<int64>(const Falcon::Item& itm)
{
    return (int64)itm.forceInteger();
}

template <> inline bool ConvertFalconItem<bool>(const Falcon::Item& itm)
{
    return itm.isTrue();
}

template <typename T> class FalconProxy;
template <typename T, typename PROXY = FalconProxy<T> > class FalconCarriedObject;


#if 0

// TODO: needs just a few more fixes to be usable!

// The actual object that is passed around in the falcon VM
// TODO: try to use CoreObject instead of CacheObject, may use less RAM and less overhead...
//       OTOH, CoreObject is less flexible and has no property overriding.
template <typename T> class FalconProxy : public Falcon::CacheObject
{
public:
    FalconProxy(Falcon::CoreClass *generator, FalconCarriedObject<T> *c = NULL)
        : Falcon::CacheObject(generator), carried(c)
    {
    }

    virtual ~FalconProxy()
    {
        if(glock)
            delete glock;
        _delCarried();
    }

    inline FalconProxy *setDel(bool d = true) { del = d; return this; }
    inline FalconProxy *setReferenced(bool r = true) { referenced = r; return this; }
    inline FalconProxy *makeProtected(void)
    {
        if(!glock)
            glock = new Falcon::GarbageLock(Falcon::Item(this));
        return this;
    }
    inline FalconProxy& setCarriedObj(FalconCarriedObject<T> *c)
    {
        ASSERT(carried == NULL); // be sure this is done only once
        carried = c;
        return this;
    }

    inline       T *getCarried(void)       { return carried; }
    inline const T *getCarried(void) const { return carried; }
    virtual bool finalize()
    {
        _delCarried();
        return false;
    }

    // --- override pure virtuals ---
    virtual Falcon::CoreObject *clone() const
    {
        return NULL;
    }

private:
    FalconCarriedObject<T> *carried; // FIXME: template defaults to FalconProxy<T> -- can this cause trouble?
    Falcon::GarbageLock *glock;
    bool del;
    bool referenced; // true if the carried object holds a pointer to this.

    inline void _delCarried()
    {
        if(carried)
        {
            if(del)
                delete carried;
            else if(referenced)
                carried->_unlink();
            carried = NULL;
        }
    }
};


// adapter class every object should derive from that can somehow be passed around in the Falcon VM
template <typename T, typename PROXY /* = FalconProxy<T>*/ > class FalconCarriedObject
{
public:
    FalconCarriedObject() : _carrier(NULL) {}

    inline Falcon::CoreObject *getCarrierIfExists() const
    {
        return _carrier;
    }

    // Return any carrier, as long as it was created earlier
    inline Falcon::CoreObject *getCarrier(Falcon::CoreClass *cls) const
    {
        return _carrier ? _carrier : getTempCarrier(cls);
    }


    // Return a carrier used for script-only objects. Stays alive as long as the object is referenced.
    // The object is deleted when the carrier is garbaged.
    inline Falcon::CoreObject *getScriptCarrier(Falcon::CoreClass *cls)
    {
        return _carrier ? _carrier : (_carrier = (new PROXY(cls, this))->setDel()->setReferenced());
    }

    // Return a carrier used for objects that are stored elsewhere, and scripts may lose reference.
    // The object will never be garbaged and must be deleted manually
    inline Falcon::CoreObject *getPersistentCarrier(Falcon::CoreClass *cls)
    {
        return _carrier ? _carrier : (_carrier = (new PROXY(cls, this))->setReferenced()->makeProtected());
    }

    // Return a temporary carrier that will be garbaged when not referenced.
    // The object will not be deleted when the carrier is garbaged.
    inline Falcon::CoreObject *getTempCarrier(Falcon::CoreClass *cls) const
    {
        return new PROXY(cls, this)
    }

    inline void _unlink()
    {
        _carrier = NULL;
    }

private:
    Falcon::CoreObject *_carrier;
};

#endif


#endif
