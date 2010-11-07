#include "common.h"
#include "EditorEngine.h"
#include "MapFile.h"
#include "TileLayer.h"
#include "LayerMgr.h"
#include "LVPAFile.h"
#include "VFSFileLVPA.h"
#include "ByteBuffer.h"
#include "LayerPanel.h"


void EditorEngine::SaveCurrentMapAs(const char *fn)
{
    VFSFile *vf = resMgr.vfs.GetFile(fn);
    if(!vf)
    {
        MapFile::SaveAsFileDirect(fn, _layermgr);
        return;
    }

    ByteBuffer bb;
    MapFile::Save(&bb, _layermgr);
    vf->open(NULL, "wb");
    vf->write((char*)bb.contents(), bb.size());
    vf->close();
    if(!strcmp(vf->getSource(), "LVPA")) // is file in archive?
    {
        
        // if so, we have to update the archive
        LVPAFile *lvpa = ((VFSFileLVPA*)vf)->getLVPA();
        logdebug("File is in container '%s', saving it", lvpa->GetMyName());
        lvpa->Save();
    }


}

bool EditorEngine::LoadMapFile(const char *fn)
{
    memblock *mb = resMgr.LoadFile((char*)fn);
    if(!mb)
    {
        logerror("EditorEngine::LoadMapFile: File not found: '%s'", fn);
        return false;
    }

    // reset camera
    _cameraPos.x = 0;
    _cameraPos.y = 0;

    LayerMgr *mgr = MapFile::Load(mb, this);
    resMgr.Drop(mb, true); // have to delete this file from memory immediately
    if(!mgr)
    {
        logerror("EditorEngine::LoadMapFile: Error loading file: '%s' as map", fn);
        return false;
    }

    // initialize missing layers
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = mgr->GetLayer(i);
        if(!layer)
        {
            layer = mgr->CreateLayer();
            mgr->SetLayer(mgr->CreateLayer(), i);
        }

        //layer->visible_area = NULL; // DEBUG
    }

    LayerMgr *oldmgr = _layermgr;
    ChangeLayerMgr(mgr);
    delete oldmgr;

    panLayers->UpdateSelection();

    return true;
}
