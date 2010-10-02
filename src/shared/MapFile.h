#ifndef MAPFILE_H
#define MAPFILE_H

class LayerMgr;
class ByteBuffer;
class Engine;

class MapFile
{
public:
    static void Save(ByteBuffer* bufptr, LayerMgr *mgr);
    static bool SaveAs(const char *fn, LayerMgr *mgr);
    static LayerMgr *LoadUnsafe(ByteBuffer *bufptr, Engine *engine); // may throw ByteBufferException if file corrupt
    static LayerMgr *Load(ByteBuffer *bufptr, Engine *engine);
    static LayerMgr *Load(memblock* mem,  Engine *engine);


};


#endif
