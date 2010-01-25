#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "common.h"
#include "ResourceMgr.h"
#include "SoundCore.h"

void musicFinished(void)
{
    // from the manual:
    // NOTE: NEVER call SDL_Mixer functions, nor SDL_LockAudio, from a callback function. 
    // oh well... works. bah.
    Mix_PlayMusic(sndCore._GetMusicPtr(), 0);
    Mix_SetMusicPosition(8.179); // TODO: TEST VALUE!!
}

SoundFile::SoundFile(Mix_Chunk *p)
: sound(p)
{
}

SoundFile::~SoundFile()
{
}

void SoundFile::SetVolume(uint8 vol)
{
    Mix_VolumeChunk(sound, vol);
}


SoundCore::SoundCore()
: _music(NULL)
{
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(16);
}

SoundCore::~SoundCore()
{
    Mix_CloseAudio();
}

void SoundCore::PlayMusic(char *fn, double repeat_pos /* = 0.0*/)
{
    if(Mix_PlayingMusic())
        StopMusic();
    _music = resMgr.LoadMusic(fn);
    if(!_music)
        return;
    Mix_PlayMusic(_music, 0); // TODO: instead of loops implement some auto-reposition system
                              // to continue playing from a specific position
    Mix_HookMusicFinished(musicFinished);
}

void SoundCore::StopMusic(void)
{
    Mix_HaltMusic();
    _music = NULL;
}

void SoundCore::SetMusicVolume(uint8 vol)
{
    Mix_VolumeMusic(vol);
}


// extern, global (since we aren't using singletons here)
SoundCore sndCore;
