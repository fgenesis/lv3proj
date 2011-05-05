#ifndef SOUNDCORE_H
#define SOUNDCORE_H

#include <SDL/SDL_mixer.h>
#include "ResourceMgr.h"
#include "SelfRefCounter.h"
#include "DelayedDeletable.h"

struct gme_t;

class SoundFile : public DelayedDeletable
{
    friend class SoundCore;

public:
    virtual ~SoundFile();
    void Play(void); // also used to resume
    void Stop(void);
    void SetVolume(uint8 vol);
    uint8 GetVolume(void);
    bool IsPlaying(void);

    //void Seek(uint32); // NYI

    SelfRefCounter<SoundFile, false> ref;

    // from DelayedDeletable
    virtual bool CanBeDeleted(void);
    virtual void SetDelete(void);

private:
    SoundFile(Mix_Chunk *p);
    ResourceCallback<Mix_Chunk> resCallback;
    Mix_Chunk *sound;
    int channel;
};

class SoundCore
{
public:
    void Init(void);
    void Destroy(void);
    SoundFile *GetSound(const char *fn); // do NOT forget to decRef the returned ptr !!
    bool PlayMusic(const char *fn);
    void PauseMusic(void);
    void StopMusic(void);
    bool IsPlayingMusic(void);
    void SetMusicVolume(uint8 vol);
    uint32 GetMusicVolume(void);

    inline Mix_Music *_GetMusicPtr(void) { return _music; }
    inline void SetLoopPoint(double msec) { _looppoint = msec; }
    inline double GetLoopPoint(void) { return _looppoint; }

private:
    bool _LoadWithGME(memblock *mb);
    Mix_Music *_music;
    gme_t *_gme;
    double _looppoint;
    uint8 _volume;

};

extern SoundCore sndCore;


#endif
