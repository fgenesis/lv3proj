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
: sound(p), channel(-1), resCallback(p), ref(this)
{
    resMgr.pool.Add(this);
}

SoundFile::~SoundFile()
{
    Stop();
    // resource management done by ResourceCallback
}

void SoundFile::SetVolume(uint8 vol)
{
    Mix_VolumeChunk(sound, vol);
}

uint8 SoundFile::GetVolume(void)
{
    return Mix_VolumeChunk(sound, -1);
}

void SoundFile::Play(void)
{
    channel = Mix_PlayChannel(-1, sound, 0);
    //DEBUG(logdebug("PLAY on channel %u", channel));
}

bool SoundFile::IsPlaying(void)
{
    return channel >= 0 && Mix_Playing(channel) && Mix_GetChunk(channel) == sound;
}

void SoundFile::Stop(void)
{
    //if(IsPlaying())
        Mix_HaltChannel(channel);
}

bool SoundFile::CanBeDeleted(void)
{
    return !(ref.count() || IsPlaying());
}

void SoundFile::SetDelete(void)
{
    Stop();
}

SoundCore::SoundCore()
: _music(NULL)
{
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(16);
}

void SoundCore::Destroy()
{
    StopMusic();
    Mix_CloseAudio();
}

void SoundCore::PlayMusic(char *fn)
{
    if(!fn)
    {
        if(Mix_PausedMusic())
            Mix_ResumeMusic();
        return;
    }
    if(Mix_PlayingMusic())
        StopMusic();
    _music = resMgr.LoadMusic(fn);
    if(!_music)
        return;
    SetLoopPoint(atof(resMgr.GetPropForMusic(fn, "looppoint").c_str()));
    Mix_PlayMusic(_music, 0);
    Mix_HookMusicFinished(musicFinished);
}
void SoundCore::PauseMusic(void)
{
    Mix_PauseMusic();
}

void SoundCore::StopMusic(void)
{
    if(_music)
    {
        Mix_HaltMusic();
        resMgr.Drop(_music);
        _music = NULL;
    }
}

bool SoundCore::IsPlayingMusic(void)
{
    return Mix_PlayingMusic() && !Mix_PausedMusic();
}

void SoundCore::SetMusicVolume(uint8 vol)
{
    Mix_VolumeMusic(vol);
}

uint32 SoundCore::GetMusicVolume(void)
{
    return Mix_VolumeMusic(-1);
}

SoundFile *SoundCore::GetSound(char *fn)
{
    Mix_Chunk *chunk = resMgr.LoadSound(fn);
    if(chunk)
    {
        SoundFile *sound = new SoundFile(chunk);
        return sound;
    }
    return NULL;
}


// extern, global (since we aren't using singletons here)
SoundCore sndCore;
