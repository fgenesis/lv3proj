#include "common.h"
#include "ByteBuffer.h"
#include "TileLayer.h"
#include "LayerMgr.h"
#include "MapFile.h"


bool MapFile::SaveAsFileDirect(const char *fn, LayerMgr *mgr)
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
    ByteBuffer strdataBuf;
    uint32 gfxIndex;

    // append string data
    for(std::map<std::string, std::string>::iterator it = mgr->stringdata.begin(); it != mgr->stringdata.end(); ++it)
        strdataBuf << it->first << it->second;

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

                        layerbuf << uint16(usedId);
                    }
                    else // no tile there
                    {
                        layerbuf << uint16(0);
                    }



        }
    }

    uint32 prealloc = 100; // for headers and different stuff. TODO: predict better.
    prealloc += gfxBuf.size();
    prealloc += strdataBuf.size();
    prealloc += (usedLayers.size() * bytesPerLayer);


    // we have all layers now, prepare output buffer
    outbuf.resize(prealloc);
    outbuf.wpos(0);

    outbuf.append("LVPM", 4); // magic
    outbuf << uint32(1); // version
    outbuf << uint32(0) << uint32(0) << uint32(0) << uint32(0); // reserved (header + flags)

    // #1 -- string data offset (can be 0)
    uint32 strdataOffsPos = outbuf.wpos();
    outbuf << uint32(0); // stringdata offset

    // #2 -- tile names offset (must exist)
    uint32 tileOffsPos = outbuf.wpos();
    outbuf << uint32(0); // tile data offset

    // #3 -- map/layer data offset (must exist)
    uint32 dataHdrOffsPos = outbuf.wpos();
    outbuf << uint32(0); // map data header start offset

    // #4 - #12 -- reserved
    outbuf << uint32(0) << uint32(0) << uint32(0) << uint32(0); // reserved (other offsets)
    outbuf << uint32(0) << uint32(0) << uint32(0) << uint32(0); // makes total 12 offset uint32s
    outbuf << uint32(0);

    // TODO: more header data here?

    // add stringdata - if empty, at strdataOffsPos will be 0, which means no stringdata
    if(mgr->stringdata.size())
    {
        outbuf.put<uint32>(strdataOffsPos, outbuf.wpos());
        outbuf << uint32(mgr->stringdata.size()); // string count, not byte count
        outbuf.append(strdataBuf.contents(), strdataBuf.size());
    }

    // fill tiles
    outbuf.put<uint32>(tileOffsPos, outbuf.wpos()); // fix offset
    outbuf << uint32(usedGfx.size());
    outbuf.append(gfxBuf.contents(), gfxBuf.size());

    // add map data header
    outbuf.put<uint32>(dataHdrOffsPos, outbuf.wpos()); // fix offset
    outbuf << uint32(mgr->GetMaxDim()); // x width
    outbuf << uint32(mgr->GetMaxDim()); // y height
    outbuf << uint32(LAYER_MAX);
    outbuf << uint32(usedLayers.size());
    outbuf << uint32(16); // tile size x // TODO: implement for other sizes
    outbuf << uint32(16); // tile size y

    // add all layer names
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = mgr->GetLayer(i);
        outbuf << (layer ? layer->name : "");
    }

    // add individual layers
    for(std::map<uint32, ByteBuffer>::iterator it = usedLayers.begin(); it != usedLayers.end(); it++)
    {
        outbuf << uint32(it->first);
        ByteBuffer& layerbuf = it->second;
        outbuf.append(layerbuf.contents(), layerbuf.size());
    }
}

LayerMgr *MapFile::Load(memblock *mem, Engine *engine, LayerMgr *mgr /* = NULL */)
{
    // TODO: make this more efficient
    ByteBuffer bb(mem->size);
    bb.append(mem->ptr, mem->size);
    return Load(&bb, engine, mgr);
}

LayerMgr *MapFile::Load(ByteBuffer *bufptr, Engine *engine, LayerMgr *mgr /* = NULL */)
{
    try
    {
        return LoadUnsafe(bufptr, engine, mgr);
    }
    catch(ByteBufferException ex)
    {
        logerror("MapFile::Load: Exception when loading file!");
        logerror("ByteBufferException: action: '%s', rpos: %u, wpos: %u, cursize: %u, readsize: %u",
            ex.action, ex.rpos, ex.wpos, ex.cursize, ex.readsize);
    }
    return NULL;
}


LayerMgr *MapFile::LoadUnsafe(ByteBuffer *bufptr, Engine *engine, LayerMgr *mgr)
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
    buf >> pad >> pad >> pad >> pad; // reserved (other header fields + flags)

    // #1 -- string data offset (can be 0)
    uint32 strdataOffs;
    buf >> strdataOffs;

    // #2 -- tile names offset (must exist)
    uint32 tileOffs;
    buf >> tileOffs;
    if(!tileOffs) // must have tiles
        return NULL;

    // #3 -- map/layer data offset (must exist)
    uint32 dataHdrOffs;
    buf >> dataHdrOffs;
    if(!dataHdrOffs)
        return NULL; // must have a data hdr

    // #4 - #12 -- reserved
    buf >> pad >> pad >> pad >> pad; // reserved (other header fields + flags)
    buf >> pad >> pad >> pad >> pad; // total 12 offset uint32
    buf >> pad;

    // read stringdata (use extra map instead of writing into the mgr directly - loading may still fail below)
    std::map<std::string, std::string> stringdata;
    if(strdataOffs)
    {
        buf.rpos(strdataOffs);
        uint32 entries;
        buf >> entries;
        std::string key, val;
        while(entries--)
        {
            buf >> key;
            buf >> val;
            stringdata.insert(std::make_pair(key,val));
        }
    }

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
    uint32 width, height, layersTotal, layersUsed, tileSizeX, tileSizeY;
    buf >> width;
    buf >> height;
    buf >> layersTotal;
    buf >> layersUsed;
    buf >> tileSizeX;
    buf >> tileSizeY;

    // unsupported
    if(layersTotal > LAYER_MAX)
        return NULL;

    // no layers?!
    if(!layersUsed)
        return NULL;

    // bullshit data
    if(layersTotal < layersUsed)
        return NULL;

    if(mgr)
    {
        // mgr already in use, just clear existing layers
        for(uint32 i = 0; i < LAYER_MAX; ++i)
            if(TileLayer *ly = mgr->GetLayer(i))
                ly->Clear();                
    }
    else
    {
        mgr = new LayerMgr(engine);
    }
    mgr->SetMaxDim(std::max(width, height));

    uint32 layerIndex;
    uint32 tileIdx;

    // read names and create layers
    for(uint32 i = 0; i < LAYER_MAX; i++) // create as many layers as the engine supports
    {
        TileLayer *layer = mgr->GetLayer(i);
        if(!layer)
            layer = mgr->CreateLayer();
        if(i < layersTotal) // and fill the names of as many as the file has
        {
            std::string name;
            buf >> name;
            layer->name = name;
        }
        mgr->SetLayer(layer, i);
    }
    
    // read individual layer data
    for(uint32 i = 0; i < layersUsed; i++)
    {
        buf >> layerIndex;

        TileLayer *layer = mgr->GetLayer(layerIndex);

        // read tiles on layer
        for(uint32 y = 0; y < height; y++)
            for(uint32 x = 0; x < width; x++)
            {
                tileIdx = buf.read<uint16>();
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

    // assign stringdata
    mgr->stringdata = stringdata;

    return mgr;
}
