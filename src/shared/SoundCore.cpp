#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

// Game_Music_Emu
#include "gme.h"
#include "Music_Emu.h"

#include "common.h"
#include "ResourceMgr.h"
#include "SoundCore.h"

static void musicFinished(void)
{
    // from the manual:
    // NOTE: NEVER call SDL_Mixer functions, nor SDL_LockAudio, from a callback function. 
    // oh well... works. bah.
    DEBUG(logdebug("SoundCore: Music finished, looppoint = %f", (float)sndCore.GetLoopPoint()));
    if(sndCore._GetMusicPtr() && sndCore.GetLoopPoint() >= 0)
    {
        Mix_PlayMusic(sndCore._GetMusicPtr(), 0);
        Mix_SetMusicPosition(sndCore.GetLoopPoint());
    }
}

static void play_music_gme(void *udata, Uint8 *stream, int len)
{
    gme_t *gme = (gme_t*)udata;
    gme->play(len / sizeof(Music_Emu::sample_t), (Music_Emu::sample_t*)stream);
}

static void stop_music_gme(void *userdata)
{
    DEBUG(logdebug("SoundCore: delete GME, mem ptr "PTRFMT, userdata));
    memblock *mb = (memblock*)userdata;
    resMgr.Drop(mb);
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

void SoundCore::Init(void)
{
    _music = NULL;
    _gme = NULL;
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_AllocateChannels(16);
}

void SoundCore::Destroy(void)
{
    StopMusic();
    Mix_CloseAudio();
}

bool SoundCore::PlayMusic(const char *fn)
{
    if(!fn)
    {
        if(Mix_PausedMusic())
        {
            Mix_ResumeMusic();
            return true;
        }
        return false;
    }

    Mix_Music *mus = resMgr.LoadMusic(fn);
    if(mus)
    {
        std::string loopstr = resMgr.GetPropForMusic(fn, "looppoint");
        if(loopstr.length())
            SetLoopPoint(atof(loopstr.c_str()));
        else
            SetLoopPoint(-1);

        StopMusic();

        int result = Mix_PlayMusic(mus, 0);
        if(!result) // everything ok
        {
            _music = mus;
            Mix_HookMusicFinished(musicFinished);
            return true;
        }
        else
            resMgr.Drop(mus); // whoops?
    }

    // SDL_Mixer can't play it...

    // get raw memory block - should still be in memory
    std::string file("music/");
    file += fn;
    memblock *mb = resMgr.LoadFile(file.c_str());
    if(mb)
    {
        // Try GME
        if(_LoadWithGME(mb)
            // ... possibly more loaders here later ...
            )
        {
            return true; // all fine now
        }
    }

    // still no success, give up
    logerror("LoadMusic failed: '%s'", file.c_str());
    return false;
}

void SoundCore::PauseMusic(void)
{
    Mix_PauseMusic();
}

void SoundCore::StopMusic(void)
{
    Mix_HookMusic(NULL, NULL); // clear any custom player function
    Mix_HaltMusic();

    // be sure audio thread is not active
    SDL_LockAudio();
    SDL_UnlockAudio();

    if(_music)
    {
        resMgr.Drop(_music); 
        _music = NULL;
    }

    gme_delete(_gme);
    _gme = NULL;
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

SoundFile *SoundCore::GetSound(const char *fn)
{
    Mix_Chunk *chunk = resMgr.LoadSound(fn);
    if(chunk)
    {
        SoundFile *sound = new SoundFile(chunk);
        return sound;
    }
    return NULL;
}

bool SoundCore::_LoadWithGME(memblock *mb)
{
    gme_t *emu =  NULL;
    gme_err_t err = gme_open_data((void const*)mb->ptr, mb->size, &emu, 44100);
    
    if(!err)
    {
        const char *ty = gme_type_system(gme_type(emu));
        logdebug("_LoadWithGME("PTRFMT") ptr = "PTRFMT"  Type: %s" , mb, mb->ptr, ty);
        StopMusic();
        _gme = emu;
        gme_start_track(emu, 0); // TODO FIXME
        gme_set_user_cleanup(emu, stop_music_gme);
        gme_set_user_data(emu, (void*)mb);
        Mix_HookMusic(play_music_gme, (void*)emu);
        return true;
    }

    return false;
}


// extern, global (since we aren't using singletons here)
SoundCore sndCore;
