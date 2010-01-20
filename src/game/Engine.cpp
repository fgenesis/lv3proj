#include "common.h"
#include "Animparser.h"
#include "ResourceMgr.h"
#include "Engine.h"




Engine::Engine()
: _screen(NULL), _fps(0), _sleeptime(0), _quit(false), _framecounter(0)
{
    _tilemgr = new TileMgr(this);
    _fpsclock = clock();
}

Engine::~Engine()
{
    if(_screen)
        SDL_FreeSurface(_screen);
}

void Engine::SetTitle(char *title)
{
    _wintitle = title;
}

void Engine::InitScreen(uint32 sizex, uint32 sizey, uint8 bpp /* = 0 */, bool fullscreen /* = false */)
{
    _winsizex = sizex;
    _winsizey = sizey;
    uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT;
    if(fullscreen)
        flags |= SDL_FULLSCREEN;
    _screen = SDL_SetVideoMode(sizex, sizey, bpp, flags);
}

void Engine::Run(void)
{
    uint32 ms = clock();
    uint32 oldms;
    while(!_quit)
    {
        _ProcessEvents();
        oldms = ms;
        ms = clock();
        _Process(ms - oldms);
        _Render();
        _CalcFPS();
    }
}

void Engine::_ProcessEvents(void)
{
    SDL_PollEvent(&_event);
    switch(_event.type)
    {
    case SDL_QUIT:
        _quit = true;
        break;
    }
}

void Engine::_CalcFPS(void)
{
    ++_framecounter;
    uint32 ms = clock();
    if(ms - _fpsclock >= CLOCKS_PER_SEC)
    {
        char buf[100];
        _fpsclock = ms;
        _fps = _framecounter;
        _framecounter = 0;
        sprintf(buf, "%s - %u FPS", _wintitle.c_str(), _fps);
        SDL_WM_SetCaption((const char*)buf, NULL);
        if(_fps > 80)
        {
            ++_sleeptime;
        }
        else if(_sleeptime)
        {
            --_sleeptime;
        }
    }
    SDL_Delay(_sleeptime);
}

bool Engine::Setup(void)
{
    SDL_Surface *pic = IMG_Load("gfx/test.png");
    resMgr.AddResource("test.png", (void*)pic, ALLOC_FREESURFACE);

    std::deque<std::string> files = GetFileList("gfx");
    for(std::deque<std::string>::iterator it = files.begin(); it != files.end(); it++)
    {
        if(!memicmp(it->c_str() + (it->size() - 5), ".anim", 5))
        {
            std::string fn("gfx/");
            fn += *it;
            Anim *ani = LoadAnimFile((char*)fn.c_str());
            resMgr.AddResource((char*)it->c_str(), (void*)ani, ALLOC_DELETE);

            for(AnimMap::iterator am = ani->anims.begin(); am != ani->anims.end(); am++)
            {
                for(AnimFrameStore::iterator af = am->second.begin(); af != am->second.end(); af++)
                {
                    if(SDL_Surface *surface = (SDL_Surface*)resMgr.GetResource((char*)af->filename.c_str(), true))
                    {
                        af->surface = surface;
                    }
                    else
                    {
                        std::string fn2("gfx/");
                        fn2 += af->filename;
                        af->surface = IMG_Load(fn2.c_str());
                        resMgr.AddResource((char*)af->filename.c_str(), (void*)af->surface, ALLOC_FREESURFACE);
                    }
                }
            }

        }
    }

    // TEMP HACK: LETS SEE HOW IT WORKS
    SDL_Surface *blk = IMG_Load("gfx/block1.png");
    SDL_Surface *oleft = IMG_Load("gfx/outlet1.png");
    SDL_Surface *oright = IMG_Load("gfx/outlet2.png");


    _tilemgr->SetStaticTileSurface(0,20,oleft);
    _tilemgr->SetStaticTileSurface(15,20,oright);
    for(uint32 i = 1; i < 15; i++)
    {
        _tilemgr->SetAnimatedTileSurface(i,20, new AnimatedTile("en.anim"));
    }

 
    for(uint32 x = 16; x < 22; x++)
        for(uint32 y = 1; y < 6; y++)
            _tilemgr->SetStaticTileSurface(x,y,blk);

    _tilemgr->SetStaticTileSurface(17,11,oleft);
    _tilemgr->SetAnimatedTileSurface(18,11, new AnimatedTile("en.anim"));
    _tilemgr->SetAnimatedTileSurface(19,11, new AnimatedTile("en.anim"));
    _tilemgr->SetStaticTileSurface(20,11,oright);

    _tilemgr->SetStaticTileSurface(6,11,oleft);
    _tilemgr->SetAnimatedTileSurface(7,11, new AnimatedTile("en.anim"));
    _tilemgr->SetAnimatedTileSurface(8,11, new AnimatedTile("en.anim"));
    _tilemgr->SetStaticTileSurface(9,11,oright);


    _tilemgr->SetStaticTileSurface(6,27,blk);
    _tilemgr->SetStaticTileSurface(7,27,blk);
    _tilemgr->SetStaticTileSurface(8,27,blk);
    _tilemgr->SetStaticTileSurface(9,27,blk);

    _tilemgr->SetStaticTileSurface(5,28,oleft);
    _tilemgr->SetAnimatedTileSurface(6,28, new AnimatedTile("en.anim"));
    _tilemgr->SetAnimatedTileSurface(7,28, new AnimatedTile("en.anim"));
    _tilemgr->SetAnimatedTileSurface(8,28, new AnimatedTile("en.anim"));
    _tilemgr->SetAnimatedTileSurface(9,28, new AnimatedTile("en.anim"));
    _tilemgr->SetStaticTileSurface(10,28,oright);


    return true;
}

void Engine::_Process(uint32 ms)
{
    _tilemgr->HandleAnimation(ms);
}

void Engine::_Render(void)
{
    // TODO: render background?
    SDL_BlitSurface((SDL_Surface*)resMgr.GetResource("test.png"), NULL, GetSurface(), NULL);

    _tilemgr->RenderStaticTiles();
    _tilemgr->RenderAnimatedTiles();
    // TODO: render sprites
    SDL_Flip(_screen);
}