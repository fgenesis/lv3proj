#include "common.h"
#include "Engine.h"
#include "ResourceMgr.h"
#include "AsciiLevelParser.h"
#include "LayerMgr.h"


LayerMgr::LayerMgr(Engine* e)
: engine(e), _maxdim(0)
{
    for(uint32 i = 0; i < LAYER_MAX; ++i)
        _layers[i] = NULL;
}

TileLayerBase *LayerMgr::CreateLayer(LayerType ty, bool collision, uint32 xoffs /* = 0 */, uint32 yoffs /* = 0 */)
{
    ASSERT(_maxdim); // sanity check

    TileLayerBase *layer;
    switch(ty)
    {
        case LAYERTYPE_BASE:
            layer = new TileLayerBase;
            layer->ty = LAYERTYPE_BASE;
            break;

        case LAYERTYPE_TILED:
            layer = new TileLayerArray2d;
            layer->ty = LAYERTYPE_TILED;
            ((TileLayerArray2d*)layer)->tilearray.resize(_maxdim, NULL);
            break;

        case LAYERTYPE_ANIMATED:
            layer = new TileLayer;
            layer->ty = LAYERTYPE_ANIMATED;
            ((TileLayer*)layer)->tilearray.resize(_maxdim, NULL);
            break;

        default:
            ASSERT(false);
    };

    ASSERT(layer);

    // create the surface where static tiles will be drawn to,
    // use the same resolution as the screen
    layer->surface = ty == LAYERTYPE_ANIMATED ? NULL :
            SDL_CreateRGBSurface(SDL_SWSURFACE, engine->GetResX(), engine->GetResY(), engine->GetBPP(), 0, 0, 0, 0);

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

bool LayerMgr::LoadAsciiLevel(AsciiLevel *level)
{
    // reserve space
    SetMaxDim(level->tiles.size1d());

    // create the layers
    TileLayerArray2d *baseLayer = (TileLayerArray2d*)CreateLayer(LAYERTYPE_TILED, false);
    TileLayer *animLayer = (TileLayer*)CreateLayer(LAYERTYPE_ANIMATED, false);
    
    // load the tiles
    std::map<std::string, BasicTile*> tmap; // stores already allocated animated tiles
    std::map<std::string, BasicTile*>::iterator it;
    std::string realFileName, startAnim;
    uint32 startIdx = 0;
    for(uint32 y = 0; y < level->tiles.size1d(); ++y)
    {
        for(uint32 x = 0; x < level->tiles.size1d(); ++x)
        {
            std::vector<std::string>& filevect = level->tiledata[level->tiles(x,y)];
            for(uint32 i = 0; i < filevect.size(); ++i)
            {
                std::string& f = filevect[i];
                AnimatedTile::SplitFilenameToProps(f.c_str(), &realFileName, &startIdx, &startAnim);
                if(FileGetExtension(realFileName) == ".png")
                {
                    it = tmap.find(f);
                    BasicTile *staTile = NULL;
                    if(it == tmap.end())
                    {
                        staTile = new BasicTile;
                        staTile->surface = resMgr.LoadImage((char*)realFileName.c_str());
                        tmap[f] = staTile;
                    }
                    else
                        staTile = it->second;

                    baseLayer->SetTile(x,y,staTile); // this will just copy the surface
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
                            tmap[f] = atile;
                        }
                        else
                            logerror("LayerMgr::LoadAsciiLevel: Error loading '%s'", realFileName.c_str());
                    }
                    else
                        atile = (AnimatedTile*)it->second;
                        
                    animLayer->SetTile(x,y,atile);
                }
            }
        }
    }

    SetLayer(baseLayer, LAYER_SPRITES);
    SetLayer(animLayer, LayerDepth(LAYER_SPRITES + 1));

    return true;
}
            