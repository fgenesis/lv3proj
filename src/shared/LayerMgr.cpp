#include <SDL/SDL.h>
#include "SDL_func.h"

#include "common.h"
#include "Engine.h"
#include "ResourceMgr.h"
#include "AsciiLevelParser.h"
#include "LayerMgr.h"


LayerMgr::LayerMgr(Engine* e)
: engine(e), _maxdim(0), _collisionMap(NULL)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _layers[i] = NULL;

}

TileLayer *LayerMgr::CreateLayer(bool collision, uint32 xoffs /* = 0 */, uint32 yoffs /* = 0 */)
{
    ASSERT(_maxdim); // sanity check

    TileLayer *layer = new TileLayer;
    layer->mgr = this;
    layer->tilearray.resize(_maxdim, NULL);
    layer->used = 0;
    layer->collision = collision;
    layer->target = engine->GetSurface();
    layer->visible_area = engine->GetVisibleBlockRect();
    layer->visible = true;
    layer->xoffs = xoffs;
    layer->yoffs = yoffs;

    return layer;
}

LayerMgr::~LayerMgr()
{
    Clear();
}

void LayerMgr::Clear(void)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(_layers[i])
        {
            delete _layers[i];
        }

        _layers[i] = NULL;
    }
    if(_collisionMap)
        delete _collisionMap;
}

void LayerMgr::Render(void)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(_layers[i])
            _layers[i]->Render();
}

void LayerMgr::Update(uint32 curtime)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(_layers[i])
            _layers[i]->Update(curtime);
}

void LayerMgr::CreateCollisionMap(void)
{
    if(_collisionMap)
        delete _collisionMap;
    _collisionMap = new BitSet2d(_maxdim * 16, _maxdim * 16); // _maxdim is tile size, * 16 is pixel size
}

// intended for initial collision map generation, NOT for regular updates! (its just too slow)
void LayerMgr::UpdateCollisionMap(void)
{
    for(uint32 y = 0; y < _maxdim; ++y)
        for(uint32 x = 0; x < _maxdim; ++x)
            UpdateCollisionMap(x,y);
}

// TODO: this can maybe be a lot more optimized...
void LayerMgr::UpdateCollisionMap(uint32 x, uint32 y) // this x and y are tile positions!
{
    DEBUG_LOG("LayerMgr::UpdateCollisionMap(%u, %u)", x, y);
    DEBUG(ASSERT(_collisionMap));
    uint32 x16 = x << 4, y16 = y << 4; // x*16, y*16
    // pre-select layers to be used, and check if a tile exists at that position
    bool uselayer[LAYER_MAX];
    bool counter = 0;
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        if(uselayer[i] = _layers[i] && _layers[i]->used && _layers[i]->collision && _layers[i]->tilearray(x,y))
            ++counter;
    if(!counter) // no layers to be used, means there is no tile here on any layer -> tile is fully passable. update all 16x16 pixels.
    {
        for(uint32 py = 0; py < 16; ++py)
            for(uint32 px = 0; px < 16; ++px)
                _collisionMap->set(x16 + px, y16 + py, false);
        return;
    }

    // lock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->surface;
            if(SDL_MUSTLOCK(surface))
                SDL_LockSurface(surface);
        }
    }
    uint32 pix;
    uint8 r, g, b, a;
    bool solid;

    for(uint32 py = 0; py < 16; ++py)
        for(uint32 px = 0; px < 16; ++px)
        {
            solid = false;
            for(uint32 i = 0; i < LAYER_MAX; ++i)
            {
                if(uselayer[i])
                {
                    BasicTile *tile = _layers[i]->tilearray(x,y);
                    pix = SDLfunc_getpixel(tile->surface, px, py);
                    SDL_GetRGBA(pix, tile->surface->format, &r, &g, &b, &a);
                    // if not fully transparent, this pixel is solid and cannot be passed
                    if(a) // TODO: maybe support that an alpha value below some threshold does NOT count as solid...?
                    {
                        solid = true;
                        break;
                    }
                }
            }
            _collisionMap->set(x16 + px, y16 + py, solid);
        }

    // unlock the SDL_Surfaces on all layers for the tile at the specified position, if required
    for(uint32 i = 0; i < LAYER_MAX; ++i)
    {
        if(uselayer[i])
        {
            SDL_Surface *surface = _layers[i]->tilearray(x,y)->surface;
            if(SDL_MUSTLOCK(surface))
                SDL_UnlockSurface(surface);
        }
    }
}

bool LayerMgr::LoadAsciiLevel(AsciiLevel *level)
{
    // reserve space
    SetMaxDim(level->tiles.size1d());

    // for the sake of loading speed in debug mode, this is not enabled for now
    //CreateCollisionMap();

    // create the layers
    TileLayer *baseLayer = CreateLayer(true);
    TileLayer *animLayer = CreateLayer();
    
    // load the tiles
    std::map<std::string, BasicTile*> tmap; // stores already allocated animated tiles
    std::map<std::string, BasicTile*>::iterator it;
    std::string realFileName, startAnim;
    uint32 startIdx = 0;
    std::string startIdxStr;
    for(uint32 y = 0; y < level->tiles.size1d(); ++y)
    {
        for(uint32 x = 0; x < level->tiles.size1d(); ++x)
        {
            std::vector<std::string>& filevect = level->tiledata[level->tiles(x,y)];
            for(uint32 i = 0; i < filevect.size(); ++i)
            {
                std::string& f = filevect[i];
                startIdxStr = "";
                startAnim = "";
                SplitFilenameToProps(f.c_str(), &realFileName, &startIdxStr, &startAnim);
                startIdx = atoi(startIdxStr.c_str());
                if(FileGetExtension(realFileName) == ".png")
                {
                    it = tmap.find(f);
                    BasicTile *staTile = NULL;
                    if(it == tmap.end())
                    {
                        staTile = new BasicTile;
                        staTile->surface = resMgr.LoadImage((char*)f.c_str());
                        staTile->filename = realFileName;
                        tmap[f] = staTile;
                    }
                    else
                        staTile = it->second;

                    baseLayer->SetTile(x,y,staTile,false); // this will just copy the surface
                }
                else if(FileGetExtension(realFileName) == ".anim")
                {
                    it = tmap.find(f);
                    AnimatedTile *atile = NULL;
                    if(it == tmap.end())
                    {
                        Anim *ani = resMgr.LoadAnim((char*)realFileName.c_str());
                        if(ani)
                        {
                            atile = new AnimatedTile(ani, startIdx, startAnim.c_str());
                            atile->Init(Engine::GetCurFrameTime());
                            atile->filename = realFileName;
                            tmap[f] = atile;
                        }
                        else
                            logerror("LayerMgr::LoadAsciiLevel: Error loading '%s'", realFileName.c_str());
                    }
                    else
                        atile = (AnimatedTile*)it->second;
                        
                    animLayer->SetTile(x,y,atile,false);
                }
            }
        }
    }

    SetLayer(baseLayer, LAYER_DEFAULT_ENV);
    SetLayer(animLayer, LAYER_DEFAULT_ENV + 1);
    SetLayer(CreateLayer(), LAYER_DEFAULT_ENV + 2); // for testing

    /* // for the sake of loading speed in debug mode, this is not enabled for now
    logdebug("Calculating collision map...");
    UpdateCollisionMap();
    */

    logdetail("ASCII Level loaded.");

    return true;
}
            