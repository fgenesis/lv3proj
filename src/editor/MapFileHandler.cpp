#include "common.h"
#include "EditorEngine.h"
#include "MapFile.h"
#include "TileLayer.h"
#include "LayerMgr.h"
#include "LVPAFile.h"
#include "VFSFileLVPA.h"
#include "ByteBuffer.h"
#include "LayerPanel.h"

void EditorEngine::_SaveCurrentMap(void)
{
    _SaveCurrentMapAs(_currentMapFileName.c_str());
}

void EditorEngine::_SaveCurrentMapAs(const char *fn)
{
    if(!(fn && *fn))
    {
        logdebug("_SaveCurrentMapAs: Empty file name!");
        GetFileDlg()->Open(true, "map"); // HACK: make this call nicer (also see TopMenus.cpp for this)
        return;
    }

    logdetail("Saving map to '%s'", fn);
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
    if(!Engine::LoadMapFile(fn))
        return false;

    // reset camera
    // TODO: move this to falcon?
    _cameraPos.x = 0;
    _cameraPos.y = 0;

    // initialize missing layers
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = _layermgr->GetLayer(i);
        if(!layer)
            layer = _layermgr->CreateLayer();

        _layermgr->SetLayer(layer, i);
    }

    panLayers->UpdateSelection();

    _currentMapFileName = fn; // for quick save without entering filename

    return true;
}
