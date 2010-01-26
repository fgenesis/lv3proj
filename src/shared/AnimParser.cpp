#include "common.h"
#include "AnimParser.h"

/*
bool IsOnlyWhitespaceOrComment(char *str)
{
    for( ; *str && (*str == ' ' || *str == '\t' || *str == 10 || *str == 13); str++)
        if(*str == '#')
            return true; // everything that comes after # is a comment
    return false;
}
*/

Anim *ParseAnimData(char *strbuf)
{
    Anim *ani = new Anim;
    std::vector<std::string> lines, fields;
    uint32 cpos; // comment?
    std::string anim_name;
    StrSplit(strbuf, "\n", lines);
    for(std::vector<std::string>::iterator lin = lines.begin(); lin != lines.end(); lin++)
    {
        if(lin->length() < 3)
            continue;

        // strip comments if there are any
        if( (cpos = lin->find('#')) != std::string::npos)
        {
            (*lin)[cpos] = '\0';
        }

        // strip whitespace at end of line
        while(lin->find_last_of(" \t") == lin->length()) // length() will never return npos so this check is fine
            (*lin)[lin->length()] = '\0';

        // check if its the animation name line - [string]
        if((*lin)[0] == '[')
        {
            anim_name = lin->c_str() + 1; // skip '['
            uint32 alen = anim_name.length();
            if(anim_name[alen - 1] == ']') // remove trailing ']'
                anim_name.erase(alen - 1);
            continue;
        }

        uint32 column = 0;
        uint32 idx;
        fields.clear();
        AnimFrameStore& frames = ani->anims[anim_name];
        StrSplit(lin->c_str(), " \t", fields);
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
                    if(frames.size() < idx + 1)
                        frames.resize(idx + 1); // make space if needed
                    frames[idx].index = idx + 1;
                }
                break;

                case 1: // filename - string
                frames[idx].filename = *it;
                break;

                case 2: // frametime - integer
                frames[idx].frametime = atoi(it->c_str());
                break;

                case 3: // nextframe - integer, OR nextanim - string
                if( !(frames[idx].nextframe = atoi(it->c_str())) )
                    frames[idx].nextanim = it->c_str();
                break;
            }
            ++column;
        }

        if(column < 4)
        {
            logerror("AnimParser: '%s' - MISSING DATA", lin->c_str()); // this is BAD!
            delete ani;
            return NULL;
        }
        else if(column > 4)
            logerror("AnimParser: '%s' - unused data left", lin->c_str()); // this is not too bad.

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

    logdebug("LoadAnimFile: '%s'", fn);

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
