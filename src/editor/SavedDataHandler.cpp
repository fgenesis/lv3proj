#include "common.h"
#include "MapFile.h"
#include "EditorEngine.h"
#include "TileboxPanel.h"


void EditorEngine::SaveData(void)
{
    // save tilebox layer
    LayerMgr mgr(this);
    mgr.SetMaxDim(panTilebox->GetTiles()[0]->GetArraySize());
    mgr.SetLayer(panTilebox->GetTiles()[0], 0);
    CreateDir("saved_data");
    CreateDir("saved_data/editor");
    bool result = MapFile::SaveAsFileDirect("saved_data/editor/last.tilebox", &mgr);
    mgr.SetLayer(NULL, 0); // detach from mgr
    if(!result)
        logerror("EditorEngine::SaveData: Failed to save 'saved_data/editor/last.tilebox'");

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
                if(TileLayer *tiles = panTilebox->GetTiles()[0])
                    delete tiles;
                panTilebox->GetTiles()[0] = tb;
                success = true;
            }
            mgr->SetLayer(NULL, 0); // detach from mgr
            delete mgr;
        }
        resMgr.Drop(mb, true); // force deletion
    }

    if(!success)
        logerror("EditorEngine::LoadData: Failed to load 'saved_data/editor/last.tilebox'");
}
