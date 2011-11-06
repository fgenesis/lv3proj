#ifndef FALCON_BASE_MODULE_H
#define FALCON_BASE_MODULE_H

#include "SharedStructs.h"

class SoundFile;
class TileLayer;
struct SDL_Surface;
namespace gcn { class Font; }


Falcon::Module *FalconBaseModule_create(void);

// this does absolutely nothing.
FALCON_FUNC fal_NullFunc(Falcon::VMachine *vm);

// these return boolean values only
FALCON_FUNC fal_TrueFunc(Falcon::VMachine *vm);
FALCON_FUNC fal_FalseFunc(Falcon::VMachine *vm);

void forbidden_init(Falcon::VMachine *vm);

class EngineError: public Falcon::Error
{
public:
    EngineError():
      Falcon::Error( "EngineError" )
      {}

      EngineError( const Falcon::ErrorParam &params  ):
      Falcon::Error( "EngineError", params )
      {}
};

class fal_Sound : public Falcon::FalconData
{
public:
    fal_Sound(SoundFile *sf);
    ~fal_Sound();
    inline SoundFile *GetSound(void) { return _snd; }
    virtual FalconData *clone() const { return NULL; } // not cloneable
    virtual void gcMark( Falcon::uint32 mark ) { }

private:
    SoundFile *_snd;
};

class fal_Font : public Falcon::FalconData
{
public:
    fal_Font(gcn::Font *f) : _font(f) {}
    ~fal_Font();
    inline gcn::Font *GetFont(void) { return _font; }
    virtual FalconData *clone() const { return NULL; } // not cloneable
    virtual void gcMark( Falcon::uint32 mark ) { }

private:
    gcn::Font *_font;
};

class fal_Surface : public Falcon::FalconObject
{
public:
    fal_Surface( const Falcon::CoreClass* generator );

    static void init(Falcon::VMachine *vm);
    static Falcon::CoreObject* factory( const Falcon::CoreClass *cls, void *user_data, bool );

    Falcon::FalconObject *clone() const;
    virtual bool finalize(void);

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value );
    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const;

    SDL_Surface *surface;
    const Camera *camera; // this camera's offsets will be added to the x/y before drawing. must be != NULL.
    bool adopted;

private:
   static const Camera s_camera; // dummy camera, for more branch-free code
};

class fal_TileLayer : public Falcon::CoreObject
{
public:
    fal_TileLayer( const Falcon::CoreClass* generator, TileLayer *obj )
        : Falcon::CoreObject( generator ), _layer(obj)
    {
    }

    static void init(Falcon::VMachine *vm);
    static Falcon::CoreObject *factory(const Falcon::CoreClass *cls, void *user_data, bool);

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value )
    {
        return false;
    }

    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const
    {
        return defaultProperty( prop, ret); // property not found
    }

    virtual Falcon::CoreObject *clone() const
    {
        return NULL; // not cloneable
    }
    inline TileLayer *GetLayer(void) { return _layer; }

private:
    TileLayer *_layer;
};

class fal_Vector2d : public Falcon::CoreObject
{
public:

    fal_Vector2d( const Falcon::CoreClass *generator, Vector2df *v = NULL )
        : Falcon::CoreObject( generator ), pVec(v ? v : &_vec)
    {
    }

    static void init(Falcon::VMachine *vm);
    static Falcon::CoreObject *factory(const Falcon::CoreClass *cls, void *user_data, bool);

    virtual bool setProperty( const Falcon::String &prop, const Falcon::Item &value );
    virtual bool getProperty( const Falcon::String &prop, Falcon::Item &ret ) const;

    virtual Falcon::CoreObject *clone(void) const;

    inline       Vector2df& vec()       { return *pVec; }
    inline const Vector2df& vec() const { return *pVec; }

private:

    Vector2df *pVec;
    Vector2df _vec; // internal vector, pVec points either to this, or another external one that is just referenced
};

// HACK: this is ugly, but works.
// maybe going to replace this with custom VM class member accessors later
// these are passed to FalconCarriedObject::get*Carrier(...) as-is.
#define VECTOR_CLASS_SYMBOL LookupFalconClass(vm, "Vector")
#define VECTOR_CLASS_SYMBOL2(VM) LookupFalconClass(VM, "Vector")

#endif
