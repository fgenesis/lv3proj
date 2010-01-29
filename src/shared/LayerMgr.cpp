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

TileLayerBase *LayerMgr::CreateLayer(LayerType ty, bool collision)
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

    // create the surface where static tiles will be drawn to,
    // use the same resolution as the screen
    layer->surface = ty == LAYERTYPE_ANIMATED ? NULL :
            SDL_CreateRGBSurface(/*SDL_HWSURFACE | SDL_HWACCEL*/ 0, engine->GetResX(), engine->GetResY(), engine->GetBPP(), 0, 0, 0, 0);

    layer->collision = collision;
    layer->target = engine->GetSurface();
    layer->visible_area = engine->GetVisibleBlockRect();

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
            SDL_FreeSurface(_layers[i]->surface);
            // TODO: delete stuff from TileLayerArray2d::tilearray ?
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
    std::map<std::string, AnimatedTile*> atmap; // stores already allocated animated tiles
    std::map<std::string, AnimatedTile*>::iterator it;
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
                if(realFileName.size() >= 4 && realFileName.substr(realFileName.size() - 4, 4) == ".png")
                {
                    baseLayer->SetTile(x,y,resMgr.LoadImage((char*)realFileName.c_str()));
                }
                else if(realFileName.size() >= 5 && realFileName.substr(realFileName.size() - 5, 5) == ".anim")
                {
                    it = atmap.find(f);
                    AnimatedTile *atile = NULL;
                    if(it == atmap.end())
                    {
                        Anim *ani = resMgr.LoadAnim((char*)realFileName.c_str());
                        if(ani)
                        {
                            atile = new AnimatedTile(ani, startIdx, startAnim.c_str());
                            atile->Init(Engine::GetCurFrameTime());
                            atmap[f] = atile;
                        }
                        else
                            logerror("LayerMgr::LoadAsciiLevel: Error loading '%s'", realFileName.c_str());
                    }
                    else
                        atile = it->second;
                        
                    animLayer->SetTile(x,y,atile);
                }
            }
        }
    }

    SetLayer(baseLayer, LAYER_SPRITES);
    SetLayer(animLayer, LayerDepth(LAYER_SPRITES + 1));

    return true;
}
            