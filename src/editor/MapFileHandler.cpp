#include "common.h"
#include "EditorEngine.h"
#include "MapFile.h"
#include "TileLayer.h"
#include "LayerMgr.h"

void EditorEngine::SaveCurrentMapAs(const char *fn)
{
    MapFile::SaveAs(fn, _layermgr);
}

bool EditorEngine::LoadMapFile(const char *fn)
{
    memblock *mb = resMgr.LoadFile((char*)fn);
    if(!mb)
    {
        logerror("EditorEngine::LoadMapFile: File not found: '%s'", fn);
        return false;
    }

    LayerMgr *mgr = MapFile::Load(mb, this);
    resMgr.Drop(mb, true); // have to delete this file from memory immediately
    if(!mgr)
    {
        logerror("EditorEngine::LoadMapFile: Error loading file: '%s'", fn);
        return false;
    }

    // initialize missing layers
    for(uint32 i = 0; i < LAYER_MAX; i++)
        if(!mgr->GetLayer(i))
            mgr->SetLayer(mgr->CreateLayer(), i);

    delete _layermgr;
    _layermgr = mgr;

    return true;
}
