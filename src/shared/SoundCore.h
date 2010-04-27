#ifndef SOUNDCORE_H
#define SOUNDCORE_H

#include <SDL/SDL_mixer.h>


class SoundFile
{
public:
    SoundFile(Mix_Chunk *p);
    ~SoundFile();
    void Pause(void);
    void Play(void); // also used to resume
    void Stop(void);
    void SetVolume(uint8 vol);

    //void Seek(uint32); // NYI

private:
    Mix_Chunk *sound;
};

class SoundCore
{
public:
    SoundCore();
    ~SoundCore();
    void PlaySound(char *fn);
    void PlayMusic(char *fn, double repeat_pos = 0.0);
    void StopMusic();
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
