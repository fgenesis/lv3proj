#include "common.h"
#include "ByteBuffer.h"
#include "TileLayer.h"
#include "LayerMgr.h"
#include "MapFile.h"


bool MapFile::SaveAs(const char *fn, LayerMgr *mgr)
{
    ByteBuffer bb;
    Save(&bb, mgr);
    if(!bb.size())
        return false;
    FILE *fh = fopen(fn, "wb");
    if(!fh)
        return false;

    uint32 bytes = fwrite(bb.contents(), 1, bb.wpos(), fh); // wpos is effective data size here
    fclose(fh);

    return bytes == bb.wpos(); // write successful?
}

void MapFile::Save(ByteBuffer *bufptr, LayerMgr *mgr)
{
    ByteBuffer& outbuf = *bufptr;

    uint32 entriesPerLayer = mgr->GetMaxDim() * mgr->GetMaxDim();
    uint32 bytesPerLayer = entriesPerLayer * sizeof(uint32);
    std::map<std::string, uint32> usedGfx;
    std::map<uint32, ByteBuffer> usedLayers;
    ByteBuffer gfxBuf;
    uint32 gfxIndex;

    // entry with ID 0 is always empty string
    gfxBuf << "";
    gfxIndex = 1;


    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = mgr->GetLayer(i);
        if(layer && layer->IsUsed())
        {
            ByteBuffer& layerbuf = usedLayers[i];
            layerbuf.reserve(bytesPerLayer);

            for(uint32 y = 0; y < layer->GetArraySize(); y++)
                for(uint32 x = 0; x < layer->GetArraySize(); x++)
                    if(BasicTile *tile = layer->GetTile(x,y))
                    {
                        std::string gfx = tile->GetFilename();
                        std::map<std::string, uint32>::iterator it = usedGfx.find(gfx);
                        uint32 usedId;
                        if(it == usedGfx.end()) // gfx not found
                        {
                            usedGfx[gfx] = gfxIndex;
                            gfxBuf << gfx;
                            usedId = gfxIndex;
                            ++gfxIndex;
                        }
                        else
                            usedId = it->second;

                        layerbuf << usedId;
                    }
                    else // no tile there
                    {
                        layerbuf << uint32(0);
                    }



        }
    }

    uint32 prealloc = 100; // for headers and different stuff. TODO: predict better.
    prealloc += gfxBuf.size();
    prealloc += (usedLayers.size() * bytesPerLayer);


    // we have all layers now, prepare output buffer
    outbuf.resize(prealloc);
    outbuf.wpos(0);

    outbuf.append("LVPM", 4); // magic
    outbuf << uint32(1); // version
    outbuf << uint32(0) << uint32(0) << uint32(0) << uint32(0); // reserved

    uint32 tileOffsPos = outbuf.wpos();
    outbuf << uint32(0); // tile data offset

    uint32 dataHdrOffsPos = outbuf.wpos();
    outbuf << uint32(0); // map data header start offset

    // TODO: more header data here?

    // fill tiles
    outbuf.put<uint32>(tileOffsPos, outbuf.wpos()); // fix offset
    outbuf << uint32(usedGfx.size());
    outbuf.append(gfxBuf.contents(), gfxBuf.size());

    // add map data header
    outbuf.put<uint32>(dataHdrOffsPos, outbuf.wpos()); // fix offset
    outbuf << uint32(mgr->GetMaxDim()); // x width
    outbuf << uint32(mgr->GetMaxDim()); // y height
    outbuf << uint32(usedLayers.size());

    // add individual layers
    for(std::map<uint32, ByteBuffer>::iterator it = usedLayers.begin(); it != usedLayers.end(); it++)
    {
        outbuf << uint32(it->first);
        ByteBuffer& layerbuf = it->second;
        outbuf.append(layerbuf.contents(), layerbuf.size());
    }
}

LayerMgr *MapFile::Load(memblock *mem, Engine *engine)
{
    // TODO: make this more efficient
    ByteBuffer bb(mem->size);
    bb.append(mem->ptr, mem->size);
    return Load(&bb, engine);
}


LayerMgr *MapFile::Load(ByteBuffer *bufptr, Engine *engine)
{
    ByteBuffer& buf = *bufptr;

    char magic[4];
    buf.read((uint8*)(&magic[0]), 4);
    if(memcmp(magic, "LVPM", 4))
        return NULL;

    uint32 version;
    buf >> version;
    if(version != 1)
        return NULL;

    uint32 pad;
    buf >> pad >> pad >> pad >> pad; // reserved

    uint32 tileOffs;
    buf >> tileOffs;

    uint32 dataHdrOffs;
    buf >> dataHdrOffs;

    // read gfx strings
    buf.rpos(tileOffs);
    uint32 strCount;
    buf >> strCount;
    if(!strCount)
        return NULL;
    std::vector<std::string> gfxStore(strCount + 1); // +1 because of the initial empty string at position 0
    for(uint32 i = 0; i < gfxStore.size(); i++)
    {
        buf >> gfxStore[i];
    }

    // read layers header
    buf.rpos(dataHdrOffs);
    uint32 width, height, layerCount;
    buf >> width;
    buf >> height;
    buf >> layerCount;

    if(!layerCount)
        return NULL;

    LayerMgr *mgr = new LayerMgr(engine);
    mgr->SetMaxDim(std::max(width, height));

    uint32 layerIndex;
    uint32 tileIdx;
    
    // read individual layers
    for(uint32 i = 0; i < layerCount; i++)
    {
        buf >> layerIndex;

        TileLayer *layer = mgr->CreateLayer();
        mgr->SetLayer(layer, layerIndex);

        // read tiles on layer
        for(uint32 y = 0; y < height; y++)
            for(uint32 x = 0; x < width; x++)
            {
                buf >> tileIdx;
                if(!tileIdx || tileIdx >= gfxStore.size())
                    continue;

                std::string& gfxName = gfxStore[tileIdx];

                if(BasicTile *tile = AnimatedTile::New(gfxName.c_str()))
                {
                    layer->SetTile(x, y, tile, false);
                    tile->ref--;
                }
            }
    }

    return mgr;
}
