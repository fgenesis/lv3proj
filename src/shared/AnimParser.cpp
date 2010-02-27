#include "common.h"
#include "AnimParser.h"


Anim *ParseAnimData(char *strbuf)
{
    Anim *ani = new Anim;
    std::vector<std::string> lines, fields;
    uint32 cpos; // comment?
    std::string anim_name;
    StrSplit(strbuf, "\n", lines);
    for(std::vector<std::string>::iterator lin = lines.begin(); lin != lines.end(); lin++)
    {
        std::string line = *lin;
        if(line.length() < 3)
            continue;

        // strip comments if there are any
        if( (cpos = line.find('#')) != std::string::npos)
        {
            line = line.substr(0, cpos);
        }

        // strip whitespace at end of line. this could be a little more efficient too
        while(line.find_last_of(" \t") == line.length()) // length() will never return npos so this check is fine
            line = line.substr(0, line.length() - 1);

        // anything left? no - line was comment only, continue with next
        if(!line.length())
            continue;

        // check if its the animation name line - [string]
        if(line[0] == '[')
        {
            anim_name = line.c_str() + 1; // skip '['
            uint32 alen = anim_name.length();
            if(anim_name[alen - 1] == ']') // remove trailing ']'
                anim_name.erase(alen - 1);
            continue;
        }

        uint32 column = 0;
        uint32 idx;
        fields.clear();
        AnimFrameStore& frames = ani->anims[anim_name];
        frames.name = anim_name;
        StrSplit(line.c_str(), " \t", fields);
        for(std::vector<std::string>::iterator it = fields.begin(); it != fields.end(); it++)
        {
            if(!it->length())
                continue;

            switch(column)
            {
                case 0: // index - integer
                {
                    uint32 x = atoi(it->c_str());
                    if(!x)
                        continue; // bullshit line, continue
                    idx = x - 1; // note the -1 here !!
                    if(frames.store.size() < idx + 1)
                        frames.store.resize(idx + 1); // make space if needed
                    frames.store[idx].index = idx + 1;
                }
                break;

                case 1: // filename - string
                frames.store[idx].filename = *it;
                break;

                case 2: // frametime - integer
                frames.store[idx].frametime = atoi(it->c_str());
                break;

                case 3: // nextframe - integer, OR nextanim - string
                if( !(frames.store[idx].nextframe = atoi(it->c_str())) )
                    frames.store[idx].nextanim = it->c_str();
                break;
            }
            ++column;
        }

        if(column < 4)
        {
            logerror("AnimParser: '%s' - MISSING DATA", line.c_str()); // this is BAD!
            delete ani;
            return NULL;
        }
        else if(column > 4)
            logerror("AnimParser: '%s' - unused data left", line.c_str()); // this is not too bad.

    }

    // TODO: sanity checks:
    // -- no used, but undefined anim names
    // -- etc

    return ani;
}

Anim *LoadAnimFile(char* fn)
{
    FILE *fh = fopen(fn, "r");
    if(!fh)
    {
        logerror("LoadAnimFile: Failed to open '%s'", fn);
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

    Anim *ani = ParseAnimData(buf);
    delete [] buf;
    return ani;
}
