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
    LAYER_REARMOST_BACKGROUND = -10,
    LAYER_SPRITES = 0,
    LAYER_FOREMOST_OVERLAY = 10,

    LAYER_MAX = LAYER_FOREMOST_OVERLAY - LAYER_REARMOST_BACKGROUND
};


class LayerMgr
{
public:

    LayerMgr(Engine*);
    ~LayerMgr();


    inline TileLayerBase *GetLayer(uint32 z) { return _layers[z]; }
    TileLayerBase *CreateLayer(LayerType ty, bool collision);
    inline void SetLayer(TileLayerBase *layer, LayerDepth depth)
    {
        _layers[depth + LAYER_FOREMOST_OVERLAY] = layer;
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
