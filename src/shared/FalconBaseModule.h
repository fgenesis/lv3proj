#ifndef FALCON_BASE_MODULE_H
#define FALCON_BASE_MODULE_H

class SoundFile;


Falcon::Module *FalconBaseModule_create(void);

// this does absolutely nothing.
FALCON_FUNC fal_NullFunc(Falcon::VMachine *vm);

// these return boolean values only
FALCON_FUNC fal_TrueFunc(Falcon::VMachine *vm);
FALCON_FUNC fal_FalseFunc(Falcon::VMachine *vm);

void forbidden_init(Falcon::VMachine *vm);

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

#endif
