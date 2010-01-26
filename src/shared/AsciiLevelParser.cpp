#include "common.h"
#include "array2d.h"
#include "AsciiLevelParser.h"


AsciiLevel *ParseAsciiLevel(char *strbuf)
{
    AsciiLevel *level = new AsciiLevel;
    std::vector<std::string> lines, files;
    std::string line;
    uint8 tiletype;
    StrSplit(strbuf, "\n", lines);

    std::vector<std::string>::iterator lin = lines.begin();

    // first, parse tile dat

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
              level->tiles.resize(maxdim);
              level->tiles.fill(0);
          }
        }

        if(line.length() > maxdim)
            line.erase(maxdim - 1, line.length() - maxdim);



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
    FILE *fh = fopen(fn, "r");
    if(!fh)
    {
        logerror("LoadAsciiLevel: Failed to open '%s'", fn);
        return NULL;
    }

    fseek(fh, 0, SEEK_END);
    uint32 size = ftell(fh);
    rewind(fh);

    char *buf = new char[size];
    uint32 bytes = fread(buf, 1, size, fh);
    ASSERT(bytes <= size);
    buf[bytes] = 0;
    fclose(fh);

    AsciiLevel *level = ParseAsciiLevel(buf);
    delete [] buf;
    return level;
}