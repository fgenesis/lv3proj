#ifndef SOUNDCORE_H
#define SOUNDCORE_H


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
    SoundFile *PlaySound(char *fn);
    void PlayMusic(char *fn, double repeat_pos = 0.0);
    void StopMusic();
    void SetMusicVolume(uint8 vol);

    inline Mix_Music *_GetMusicPtr(void) { return _music; }

private:
    Mix_Music *_music;

};

extern SoundCore sndCore;


#endif
