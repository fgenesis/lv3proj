#include "common.h"
#include "array2d.h"
#include "ResourceMgr.h"
#include "AsciiLevelParser.h"


AsciiLevel *ParseAsciiLevel(char *strbuf)
{
    AsciiLevel *level = new AsciiLevel;
    std::vector<std::string> lines, files;
    std::string line;
    uint8 tiletype;
    StrSplit(strbuf, "\n", lines);

    std::vector<std::string>::iterator lin = lines.begin();

    // first, parse tile data

    for( ; lin != lines.end(); lin++)
    {
        if(lin->empty() || (lin->at(0) == '$' && lin->at(1) == '$'))
            break;
        if(lin->length() < 3)
            continue;

        line = *lin;
        tiletype = line[0];
        line.erase(0,2);

        files.clear();
        StrSplit(line, " ", files);

        for(std::vector<std::string>::iterator it = files.begin(); it != files.end(); it++)
        {
            if(!it->length())
                continue;

            level->tiledata[tiletype].push_back(*it);
        }

        if(level->tiledata[tiletype].empty())
            break;
    }

    uint32 lineNum = 0;
    uint32 maxdim = 0;
    for( ; lin != lines.end(); lin++)
    {
        if(lin->empty())
            break;
        if(lin->length() < 3)
            continue;

        line = *lin;
        if(!maxdim)
        {
          maxdim = std::max((uint32)line.length(), lineNum);
          if(level->tiles.size1d() < maxdim)
          {
              level->tiles.resize(maxdim, 0);
          }
        }
        if(level->tiles.size1d() < lineNum || level->tiles.size1d() < line.length())
        {
            logerror("AsciiLevelParser: array2d too small, or level too large!");
            break;
        }

        for(uint32 i = 0; i < line.length(); ++i)
        {
            level->tiles(i, lineNum) = line[i];
        }

        lineNum++;
    }

    return level;
}

AsciiLevel *LoadAsciiLevel(char *fn)
{
    logdebug("LoadAsciiLevel: '%s'", fn);
    std::string filename("levels/");
    filename += fn;
    memblock *mb = resMgr.LoadTextFile((char*)filename.c_str());
    if(!mb)
    {
        logerror("LoadAsciiLevel: Failed to open '%s'", fn);
        return NULL;
    }

    AsciiLevel *level = ParseAsciiLevel((char*)mb->ptr);
    resMgr.Drop(mb);
    return level;
}