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
    if(sndCore.GetLoopPoint())
    {
        Mix_PlayMusic(sndCore._GetMusicPtr(), 0);
        Mix_SetMusicPosition(sndCore.GetLoopPoint());
    }
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
    std::string loopstr = resMgr.GetPropForMusic(fn, "looppoint");
    SetLoopPoint(atof(loopstr.c_str()));
    if(Mix_PlayingMusic())
        StopMusic();
    _music = resMgr.LoadMusic(fn); // TODO: do refcounting
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

uint32 SoundCore::GetMusicVolume(void)
{
    return Mix_VolumeMusic(-1);
}

// TODO: this is the minimal thing to play sound, must be improved and free resources after use!
void SoundCore::PlaySound(char *fn)
{
    Mix_Chunk *sound = resMgr.LoadSound(fn);
    if(sound)
    {
        Mix_PlayChannel(-1, sound, 0);
    }
}


// extern, global (since we aren't using singletons here)
SoundCore sndCore;
