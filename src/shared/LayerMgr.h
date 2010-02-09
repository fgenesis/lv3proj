#ifndef LAYERMGR_H
#define LAYERMGR_H

#include "array2d.h"
#include "Tile.h"
#include "TileLayer.h"

class Engine;
struct SDL_Surface;
struct AsciiLevel;

enum LayerDepth
{
    LAYER_REARMOST_BACKGROUND = 0,
    LAYER_DEFAULT_ENV = 6, // where most walls and basic stuff should be put. the editor will start with this layer.
    LAYER_SPRITES = 15, // the default sprite layer. should not contain
                        // anything else than sprites, because these have to be treated specially
    LAYER_FOREMOST_OVERLAY = 31,

    LAYER_MAX = 32
};


class LayerMgr
{
public:

    LayerMgr(Engine*);
    ~LayerMgr();


    inline TileLayerBase *GetLayer(uint32 depth) { return _layers[depth]; }
    TileLayerBase *CreateLayer(LayerType ty, bool collision, uint32 xoffs = 0, uint32 yoffs = 0);
    inline void SetLayer(TileLayerBase *layer, uint32 depth)
    {
        _layers[depth] = layer;
    }

    void Update(uint32 curtime);
    void Render(void);
    void Clear(void);

    inline void SetMaxDim(uint32 dim) { _maxdim = dim; }


    bool LoadAsciiLevel(AsciiLevel *level);


private:
    Engine *engine;
    TileLayerBase *_layers[LAYER_MAX];
    uint32 _maxdim; // max dimension for all created layers

};

#endif
