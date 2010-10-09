#include "common.h"
#include "EditorEngine.h"


void EditorEngine::FillUseableTiles(void)
{
    // ##### TEMP TEST DEBUG STUFF ####
    uint32 cnt = 0;
    std::list<std::string> dirs;
    dirs.push_back("ship");
    dirs.push_back("sprites");
    dirs.push_back("water");
    for(std::list<std::string>::iterator idir = dirs.begin(); idir != dirs.end(); idir++)
    {
        std::deque<std::string> files = GetFileList(std::string("gfx/") + *idir);
        for(std::deque<std::string>::iterator fi = files.begin(); fi != files.end(); fi++)
        {
            std::string fn(AddPathIfNecessary(*fi,*idir));
            BasicTile *tile = AnimatedTile::New(fn.c_str());

            if(tile)
            {
                wndTilesLayer->SetTile(cnt % (wndTilesLayer->GetArraySize() / 2), cnt / (wndTilesLayer->GetArraySize() / 2), tile);
                //panTileboxLayer->SetTile(cnt % tileboxCols, cnt / tileboxCols, tile);
                cnt++;
            }
        }
    }
    // ### end debug stuff ###
}
