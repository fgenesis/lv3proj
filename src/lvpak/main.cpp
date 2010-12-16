#include "common.h"
#include "LVPAFile.h"
#include "Crc32.h"

struct FilenameWithProps
{
    FilenameWithProps(std::string s) : level(LVPACOMP_INHERIT), solid(false), fn(s), algo(LVPAPACK_DEFAULT) {}
    std::string fn;
    std::string relPath;
    uint8 level; // LVPACOMP_INHERIT for default
    bool solid;
    uint8 algo;
};

void usage(void)
{
    printf("lvpak [-flags] MODE archive [file, ..]\n"
           "\n"
           "Modes:\n"
           "  a - append to archive or create new\n"
           //"  d - create archive from directory\n"
           //"  D - same as d, but recursive\n"
           "  e - extract to current directory\n"
           "  x - extract with full path\n"
           "  t - test archive\n"
           "  l - list files and mode\n"
           "  r - repack with a different compression level\n"
           "\n"
           "Flags:\n"
           "  -p PATH - use PATH as relative path to prepend each file (modes: aex)\n"
           //"  -P PATH - override full path\n"
           "  -c# - compression level, in range [0..9]\n"
           "  -f FILE - use a listfile\n",
           //"  -f - force operation (e.g. extract files although CRCs differ)"
           "\n"
           "<archive> is the archive file to create/modify/read\n"
           "<file,[file...]> is a list of files to add\n"
           "The default compression level is 3.\n"
           "Note that every level >= 7 takes large amounts of memory!\n"
           "\n"
           "* lvpak uses LZMA by Igor Pavlov *\n"
           );
}

void unknown(char *what)
{
    printf("Unknown parameter: '%s'\n", what);
}

bool parsecmd(int argc, char *argv[], uint8& mode, uint8& level, bool& solid, std::string& listfile, std::string& archive, std::list<FilenameWithProps>& files, std::string& relPath)
{
    if(argc < 3 && !mode)
    {
        usage();
        return false;
    }

    for(int i = 1; i < argc; ++i)
    {
        uint32 l = strlen(argv[i]);
        if(!l)
            continue;

        if(l > 1 && argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'p':
                    if(i+1 < argc)
                    {
                        relPath = argv[++i];
                        _FixFileName(relPath);
                    }
                    else
                    {
                        printf("Error: -p expects a path\n");
                        return false;
                    }
                    break;

                case 'c':
                    level = 10; // invalid setting, use as marker
                    if(l > 2)
                        level = argv[i][2] - '0'; // one-char atoi()

                    if(level > 9 && level != LVPACOMP_INHERIT)
                    {
                        printf("Error: -c expects a number in [0..9]\n");
                        return false;
                    }
                    break;

                case 's':
                    solid = true;
                    break;

                case 'f':
                    if(i+1 < argc)
                    {
                        listfile = argv[++i];
                    }
                    else
                    {
                        printf("Error: -f expects a listfile name\n");
                        return false;
                    }
                    break;


                default:
                    unknown(argv[i]);
            }
            continue;
        }

        if(l == 1 && !mode)
        {
            mode = argv[i][0];
            continue;
        }

        if(mode)
        {
            if(archive.empty())
                archive = argv[i];
            else
                files.push_back(FilenameWithProps(std::string(argv[i])));
        }
        else
        {
            printf("Unspecified mode!");
            return false;
        }

    }

    return true;

}

// modifies the input string, beware!
// argc_max is how many args we can max. parse.
uint32 splitline(char *str, uint32 argc_max, char **argv)
{
    char *s = str;
    uint32 pos = 0;
    bool inquot = false;

    // strip initial whitespace
    while(*s == ' ')
        *s++ = 0;

    char *goodpos = s;

    while(*s && pos < argc_max)
    {
        if(*s == '\"')
        {
            inquot = !inquot;
            *s = 0;
        }
        else if(!inquot && *s == ' ')
        {
            while(*s == ' ')
                *s = 0;
            argv[pos++] = goodpos;
            goodpos = s + 1;
        }
        ++s;
    }

    // last part with possibly no whitespace at end
    if(goodpos != s + 1)
        argv[pos++] = goodpos;

    return pos;
}

void buildFileList(std::list<FilenameWithProps>& files, const char *listfile, const char *relPath)
{
    FILE *fh = fopen(listfile, "r");
    if(!fh)
    {
        printf("Failed to open listfile '%s'\n", listfile);
        return;
    }

    uint32 size;

    fseek(fh, 0, SEEK_END);
    size = ftell(fh);
    fseek(fh, 0 , SEEK_SET);

    if(!size)
    {
        fclose(fh);
        return;
    }

    char *buf = new char[size + 1];
    buf[size] = 0;
    uint32 bytesRead = fread(buf, 1, size, fh);
    buf[bytesRead] = 0;
    fclose(fh);

    for(uint32 pos = 0; pos < size; pos++)
    {
        char *ptr = &buf[pos];
        for( ; pos < size && buf[pos] != 10 && buf[pos] != 13; pos++); // empty loop

        buf[pos] = 0;
        if(uint32 len = strlen(ptr))
        {
            if(ptr[0] == '#')
            {
                if(len > 9 && !memcmp("include ", &ptr[1], 8)) // check for '#include FILENAME'
                {
                    std::string path(relPath);
                    path += _PathStripLast(&ptr[9]);
                    std::string subListFile(path);
                    subListFile += _PathToFileName(&ptr[9]);
                    buildFileList(files, subListFile.c_str(), path.c_str());
                }
                continue;
            }
            char *argv[256];
            argv[0] = "";
            uint32 used = 1 + splitline(ptr, 255, &argv[1]); // +1 because of argv[0] - we dont need the exe file name here so we leave it blank

            uint8 level = LVPACOMP_INHERIT;
            bool solid = false;
            std::string dummy, fake_archive;
            std::string relPathExtra;
            std::list<FilenameWithProps> filesList;
            uint8 mode = 'a';
            if(parsecmd(used, argv, mode, level, solid, dummy, fake_archive, filesList, relPathExtra))
            {
                filesList.push_front(fake_archive); // it gets parsed as archive name (first filename) but here it should specify the fist file instead

                // add trailing '/' if necessary
                if(relPathExtra.length() && relPathExtra[relPathExtra.length() - 1] != '/')
                    relPathExtra += '/';

                for(std::list<FilenameWithProps>::iterator it = filesList.begin(); it != filesList.end(); it++)
                {
                    FilenameWithProps fp(relPath + it->fn);
                    fp.level = level;
                    fp.solid = solid;
                    fp.relPath = relPathExtra;
                    files.push_back(fp);
                }
            }
            else
            {
                printf("Malformed listfile line: '%s'\n", ptr);
                continue;
            }
        }
    }

    delete [] buf;
}


int main(int argc, char *argv[])
{
    log_setloglevel(3);
    std::string archive, relPath;
    uint8 mode = 0, level = 3;
    bool solid = false;
    std::list<FilenameWithProps> files;
    std::string errstr;
    std::string listfile;
    bool loaded, result = false;

    if(!parsecmd(argc, argv, mode, level, solid, listfile, archive, files, relPath))
    {
        return 2;
    }

    if(archive.empty())
    {
        printf("No target archive file specified!\n");
        return 2;
    }

    _FixFileName(relPath);

    if(relPath.length() && relPath[relPath.length() - 1] != '/')
        relPath += '/';

    if(listfile.length())
        buildFileList(files, listfile.c_str(), relPath.c_str());

    LVPAFile f;
    loaded = f.LoadFrom(archive.c_str(), LVPALOAD_ALL);

    bool extractFullPath = false;

    switch(mode)
    {
        case 'l':
        {
            if(!loaded)
            {
                errstr = "Error opening archive: '" + archive + "'";
                break;
            }
            for(uint32 i = 0; i < f.HeaderCount(); ++i)
            {
                const LVPAFileHeader& h = f.GetFileInfo(i);
                char lvlc;
                switch(h.level)
                {
                    case LVPACOMP_INHERIT: lvlc = 'i'; break;
                    default:               lvlc = '0' + h.level;
                }
                printf("[%c%c,%c] '%s' (%.2f%%)%s\n",
                    h.flags & LVPAFLAG_PACKED ? 'P' : '-',
                    h.flags & LVPAFLAG_SOLID ? 'S' : '-',
                    lvlc,
                    h.filename.c_str(),
                    (float(h.packedSize) / float(h.realSize)) * 100.0f,
                    h.good ? "" : " (ERROR)");
                f.Free((char*)h.filename.c_str());
            }
            printf("Total compression ratio: %u -> %u (%.2f%%)\n", f.GetRealSize(), f.GetPackedSize(),
                 float(f.GetPackedSize()) / float(f.GetRealSize()) * 100.0f);
            result = true;
            break;
        }

        case 'a':
        {
            if(files.empty())
            {
                errstr = "Add mode: no files given\n";
                break;
            }
            for(std::list<FilenameWithProps>::iterator it = files.begin(); it != files.end(); it++)
            {
                FILE *fh = fopen(it->fn.c_str(), "rb");
                if(!fh)
                {
                    printf("Add mode: file not found: '%s'\n", it->fn.c_str());
                    continue;
                }
                fseek(fh, 0, SEEK_END);
                uint32 s = ftell(fh);
                fseek(fh, 0, SEEK_SET);
                uint8 *buf = new uint8[s];
                fread(buf, s, 1, fh);
                fclose(fh);
                uint8 flags = LVPAFLAG_PACKED;
                if(it->solid || solid) // TODO: make listfile setting higher priority?
                    flags |= LVPAFLAG_SOLID;
                std::string finalFileName = relPath + it->relPath + _PathToFileName(it->fn);
                f.Add((char*)finalFileName.c_str(), memblock(buf, s), LVPAFileFlags(flags), LVPAPACK_DEFAULT, it->level);
            }
            result = f.Save(level);
            printf("Total compression ratio: %u -> %u (%.2f%%)\n", f.GetRealSize(), f.GetPackedSize(),
                float(f.GetPackedSize()) / float(f.GetRealSize()) * 100.0f);
            break;
        }

        case 'r':
        {
            if(!loaded)
            {
                errstr = "Error opening archive: '" + archive + "'";
                break;
            }
            if(f.AllGood())
            {
                printf("Before repacking: %u -> %u (%.2f%%)\n", f.GetRealSize(), f.GetPackedSize(),
                    float(f.GetPackedSize()) / float(f.GetRealSize()) * 100.0f);
                if(level == LVPACOMP_NONE)
                    printf("Repacking as uncompressed\n");
                else
                    printf("Repacking with level %d\n", level);

                result = f.Save(level);

                printf("After repacking: %u -> %u (%.2f%%)\n", f.GetRealSize(), f.GetPackedSize(),
                    float(f.GetPackedSize()) / float(f.GetRealSize()) * 100.0f);
            }
            else
                printf("File is damaged, not repacking.");
        }
        break;

        case 't':
            if(!loaded)
            {
                errstr = "Error opening archive: '" + archive + "'";
                break;
            }
            result = f.AllGood();
            printf("%s", result ? "File is OK\n" : "File is damaged, use 'lvpak l' to list\n");
            break;

        case 'x':
            extractFullPath = true;
            // fallthrough

        case 'e':
        {
            if(!loaded)
            {
                errstr = "Error opening archive: '" + archive + "'";
                break;
            }
            uint32 unpacked = 0;
            CreateDirRec(relPath.c_str());
            for(uint32 i = 0; i < f.HeaderCount(); ++i)
            {
                const LVPAFileHeader& h = f.GetFileInfo(i);
                if(!h.good)
                    continue;
                std::string exfn(relPath);
                if(extractFullPath)
                {
                    exfn += h.filename;
                    CreateDirRec(_PathStripLast(exfn).c_str());
                }
                else
                {
                    exfn += _PathToFileName(h.filename);
                }

                FILE *exfh = fopen(exfn.c_str(), "wb");
                if(!exfh)
                {
                    printf("Can't extract to file '%s'\n", exfn.c_str());
                    continue;
                }

                uint32 bytes = fwrite(h.data.ptr, 1, h.data.size, exfh);
                fclose(exfh);
                if(bytes != h.data.size)
                {
                    printf("Warning: '%s' was not fully written (%u out of %u bytes)\n", exfn.c_str(), bytes, h.data.size);
                    continue;
                }
                ++unpacked;
            }
            result = unpacked;
            break;
        }


        default:
            printf("Unsupported mode: '%c'\n", mode);
            result = false;
    }

    if(errstr.length())
        printf("%s\n", errstr.c_str());


    f.Clear();
    return result ? 0 : 1;
}

