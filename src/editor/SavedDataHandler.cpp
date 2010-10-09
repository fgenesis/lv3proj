#include "common.h"
#include "MapFile.h"
#include "EditorEngine.h"


void EditorEngine::SaveData(void)
{
    // save tilebox layer
    LayerMgr mgr(this);
    mgr.SetMaxDim(panTileboxLayer->GetArraySize());
    mgr.SetLayer(panTileboxLayer, 0);
    CreateDir("saved_data");
    CreateDir("saved_data/editor");
    bool result = MapFile::SaveAs("saved_data/editor/last.tilebox", &mgr);
    mgr.SetLayer(NULL, 0); // detach from mgr
    if(!result)
        logerror("EditorEngine::SaveData: Failed to save tilebox!");

}

void EditorEngine::LoadData(void)
{
    bool success = false;
    // load tilebox layer
    memblock *mb = resMgr.LoadFile("saved_data/editor/last.tilebox");
    if(mb)
    {
        LayerMgr *mgr = MapFile::Load(mb, this);
        if(mgr)
        {
            TileLayer *tb = mgr->GetLayer(0);
            if(tb)
            {
                tb->visible_area = NULL; // TODO: set this to something useful to speed up rendering
                tb->camera = NULL;
                if(panTileboxLayer)
                    delete panTileboxLayer;
                panTileboxLayer = tb;
                success = true;
            }
            mgr->SetLayer(NULL, 0); // detach from mgr
            delete mgr;
        }
        resMgr.Drop(mb, true); // force deletion
    }

    if(!success)
        logerror("EditorEngine::LoadData: Failed to load tilebox!");
}
