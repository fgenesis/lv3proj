#include "common.h"
#include "EditorEngine.h"
#include "VFSDir.h"
#include "VFSFile.h"
#include "ResourceMgr.h"
#include "LVPAFile.h"
#include "TileWindow.h"
#include "TileboxPanel.h"


// TODO: this function needs to be removed in some future, but it is good enough to fill up the tile list
void EditorEngine::LoadPackages(void)
{
    VFSDir *packageDir = resMgr.vfs.GetDir("packages");
    if(!packageDir)
    {
        logerror("EditorEngine::LoadPackages: Directory 'packages' not found, unable to load any packages!");
        return;
    }

    // add container files
    for(VFSFileMap::iterator it = packageDir->_files.begin(); it != packageDir->_files.end(); it++)
    {
        bool success = false;
        const char *fn = it->second->fullname();
        // does the file end with ".lvpa" ?
        uint32 len = strlen(fn);
        if(len >= 5 && !stricmp(fn + (len - 5), ".lvpa"))
        {
            // try to load as container
            LVPAFile*lvpa = new LVPAFile;
            if(lvpa->LoadFrom(fn, LVPALOAD_NONE))
            {
                success = resMgr.vfs.AddContainer(lvpa, "", true); // delete later
            }
            if(success)
                logdetail("Loaded package: '%s'", fn);
            else
            {
                logdetail("Failed to load package: '%s'", fn);
                delete lvpa;
            }
        }
    }

    // add subdirs
    for(VFSDirMap::iterator it = packageDir->_subdirs.begin(); it != packageDir->_subdirs.end(); it++)
    {
        std::string d("packages/");
        d += it->second->name();
        if(resMgr.vfs.AddPath(d.c_str()))
            logdetail("Added path: '%s'", d.c_str());
        else
            logdetail("Failed to add path '%s'", d.c_str());
    }

    resMgr.vfs.Reload();
}

void DirLoadHelper(std::string dirname, TileLayer *layer, VFSDir *dir, uint32 maxwidth, uint32& pos)
{
    if(dirname.size())
        dirname += '/';
    for(VFSFileMap::iterator it = dir->_files.begin(); it != dir->_files.end(); it++)
    {
        std::string fn = dirname + it->second->name();
        BasicTile *tile = AnimatedTile::New(fn.c_str());
        if(tile)
        {
            if(SDL_Surface *sf = tile->GetSurface())
            {
                if(sf->w == 16 && sf->h == 16) // TODO: make less hacky
                {
                    logdebug("Using '%s' as tile", fn.c_str());
                    layer->SetTile(pos % maxwidth, pos / maxwidth, tile);
                    ++pos;
                }
                else
                    logdebug("Skipping '%s' as tile, wrong size", fn.c_str());
            }

            tile->ref--;
        }
    }

    for(VFSDirMap::iterator it = dir->_subdirs.begin(); it != dir->_subdirs.end(); it++)
    {
        DirLoadHelper(dirname + it->second->name(), layer, it->second, maxwidth, pos);
    }
}

void EditorEngine::FillUseableTiles(void)
{
    VFSDir *gfxDir = resMgr.vfs.GetDir("gfx");
    if(!gfxDir)
    {
        logerror("EditorEngine::FillUseableTiles: Directory 'gfx' not in VFS!");
        return;
    }

    uint32 pos = 0;
    uint32 maxwidth = GetResX() / 16;
    DirLoadHelper("", wndTiles->GetTilesPanel()->GetTileLayer(), gfxDir, maxwidth, pos);
}
