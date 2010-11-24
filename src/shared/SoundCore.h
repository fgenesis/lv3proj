#ifndef SOUNDCORE_H
#define SOUNDCORE_H

#include <SDL/SDL_mixer.h>
#include "ResourceMgr.h"
#include "SelfRefCounter.h"


class SoundFile
{
    friend class SoundCore;

public:
    ~SoundFile();
    void Play(void); // also used to resume
    void Stop(void);
    void SetVolume(uint8 vol);
    uint8 GetVolume(void);
    bool IsPlaying(void);

    //void Seek(uint32); // NYI

    SelfRefCounter<SoundFile> ref;

private:
    SoundFile(Mix_Chunk *p);
    ResourceCallback<Mix_Chunk> resCallback;
    Mix_Chunk *sound;
    int channel;
};

class SoundCore
{
public:
    SoundCore();
    void Destroy();
    SoundFile *GetSound(char *fn); // do NOT forget to decRef the returned ptr !!
    void PlayMusic(char *fn);
    void PauseMusic(void);
    void StopMusic(void);
    bool IsPlayingMusic(void);
    void SetMusicVolume(uint8 vol);
    uint32 GetMusicVolume(void);

    inline Mix_Music *_GetMusicPtr(void) { return _music; }
    inline void SetLoopPoint(double msec) { _looppoint = msec; }
    inline double GetLoopPoint(void) { return _looppoint; }

private:
    Mix_Music *_music;
    double _looppoint;

};

extern SoundCore sndCore;


#endif
