#include "common.h"
#include "AnimParser.h"


Anim *ParseAnimData(char *strbuf, char *filename)
{
    Anim *ani = new Anim;
    ani->first = NULL;
    std::vector<std::string> lines, fields;
    uint32 cpos; // comment?
    uint32 linenum = 0; // line number
    std::string anim_name;
    bool finished_block = true;
    StrSplit(strbuf, "\n\x0d\x0a", lines, true);
    for(std::vector<std::string>::iterator lin = lines.begin(); lin != lines.end(); lin++)
    {
        linenum++;
        std::string line = *lin;
        if(line.length() < 1)
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
            finished_block = false;
            AnimFrameStore& frames = ani->anims[anim_name];
            frames.name = anim_name;
            frames.cmd.action = ANIMCMD_REPEAT;
            if(!ani->first)
                ani->first = &frames;
            continue;
        }

        if(finished_block)
        {
            logerror("AnimParser: '%s', Line %u: block finished, can't process: '%s'", filename, linenum, line.c_str());
            continue;
        }



        uint32 column = 0;
        bool isCmd = false;
        fields.clear();
        AnimFrameStore& frames = ani->anims[anim_name];
        StrSplit(line.c_str(), " \t", fields);
        for(std::vector<std::string>::iterator it = fields.begin(); it != fields.end(); it++)
        {
            if(!it->length())
                continue;

            switch(column)
            {
                case 0: // filename - string / cmd - single char
                    if(it->length() == 1)
                    {
                        isCmd = true;
                        finished_block = true;
                        switch((*it)[0])
                        {
                            case 'j': frames.cmd.action = ANIMCMD_JUMP; break;
                            case 'r': frames.cmd.action = ANIMCMD_REPEAT; break;
                            default:
                                logerror("AnimParser: '%s', Line %u: invalid action '%c', assuming 'r'", filename, linenum, (*it)[0]); 
                                frames.cmd.action = ANIMCMD_REPEAT;
                        }
                    }
                    else
                    {
                        frames.store.push_back(AnimFrame(*it, 0));
                    }
                    break;

                case 1: // optional: frametime - integer / cmd param - string
                    if(isCmd)
                    {
                        frames.cmd.param = *it;
                    }
                    else
                    {
                        frames.store.back().frametime = atoi(it->c_str());
                    }
                    break;

                default:
                    logerror("AnimParser: '%s', Line %u: unused data: '%s'", filename, linenum, it->c_str()); 
            }
            ++column;
        }

        if(frames.cmd.action == ANIMCMD_JUMP && frames.cmd.param.empty())
        {
            logerror("AnimParser: '%s', Line %u: : action 'j' requires an animation name! Set to 'r'", filename, linenum);
            frames.cmd.action = ANIMCMD_REPEAT;
        }
    }

    // sanity/error checks + cleanup
    for(AnimMap::iterator it = ani->anims.begin(); it != ani->anims.end(); it++)
    {
        AnimFrameStore &frames = it->second;

        // is a jump referring to an animation name that doesnt exist?
        if(frames.cmd.action == ANIMCMD_JUMP && ani->anims.find(it->second.cmd.param) == ani->anims.end())
        {
            logerror("AnimParser: '%s' ERROR: Animation '%s' has a jump to '%s', which does not exist! Removed.",
                filename, frames.name.c_str(), frames.cmd.param.c_str());
            frames.cmd.action = ANIMCMD_REPEAT;
            frames.cmd.param.clear();
        }
        if(frames.cmd.action == ANIMCMD_REPEAT && frames.cmd.param.size())
        {
            logerror("AnimParser: '%s' Warning: Animation '%s' has a unused param '%s', ignored.",
                filename, frames.name.c_str(), frames.cmd.param.c_str());
        }
    }
    ani->filename = filename;
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

    Anim *ani = ParseAnimData(buf, fn);
    delete [] buf;
    return ani;
}
