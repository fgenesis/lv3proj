#include "common.h"
#include "LVPAFile.h"
#include "Crc32.h"

void usage(void)
{
    printf("lvpak [-flags] [MODE] archive [file, ..]\n"
           "\n"
           "Modes:\n"
           "  a - append to archive or create new\n"
           //"  d - create archive from directory\n"
           //"  D - same as d, but recursive\n"
           //"  e - extract to current directory\n"
           //"  x - extract with full path\n"
           "  t - test archive\n"
           "  l - list files and mode\n"
           "  r - repack with a different compression level\n"
           "\n"
           "Flags:\n"
           "  -p PATH - use PATH as relative path to prepend each file\n"
           "  -c# - compression level, in range [0..9]\n"
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

bool parsecmd(int argc, char *argv[], uint8& mode, uint8& level, bool& solid, std::string& archive, std::list<std::string>& files, std::string& relPath)
{
    if(argc < 3)
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
                    }
                    else
                    {
                        printf("Error: -p expects a path\n");
                        return false;
                    }
                    break;

                case 'c':
                    level = -1;
                    if(l > 2)
                        level = argv[i][2] - '0'; // one-char atoi()

                    if(level > 9)
                    {
                        printf("Error: -c expects a number in [0..9]\n");
                        return false;
                    }
                    break;

                case 's':
                    solid = true;
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
                files.push_back(argv[i]);
        }
        else
        {
            printf("Unspecified mode!");
            return false;
        }

    }

    return true;

}


int main(int argc, char *argv[])
{
    log_setloglevel(3);
    std::string archive, relPath;
    uint8 mode = 0, level = 3;
    bool solid = false;
    std::list<std::string> files;
    std::string errstr;
    bool loaded, result = false;

    /*
#ifdef _DEBUG
    //argc = 4;
    //char *argsd[] = {"lvpakd", "a", "test.lvpa", "zlib1.dll"};
    argc = 3;
    char *argsd[] = {"lvpakd", "l", "zz.lvpa"};
    argv = argsd;
#endif
    */

    if(!parsecmd(argc, argv, mode, level, solid, archive, files, relPath))
    {
        return 2;
    }

    _FixFileName(relPath);

    if(relPath.length() && relPath[relPath.length() - 1] != '/')
        relPath += '/';

    CRC32::GenTab();

    LVPAFile f;
    loaded = f.LoadFrom(archive.c_str(), LVPALOAD_ALL);

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
                printf("[%c%c] '%s' (%.2f%%)%s\n",
                    h.flags & LVPAFLAG_PACKED ? 'P' : '-',
                    h.flags & LVPAFLAG_SOLID ? 'S' : '-',
                    h.filename.c_str(),
                    (float(h.packedSize) / float(h.realSize)) * 100.0f,
                    h.good ? "" : " (ERROR)");
                f.Free((char*)h.filename.c_str());
            }
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
            for(std::list<std::string>::iterator it = files.begin(); it != files.end(); it++)
            {
                FILE *fh = fopen(it->c_str(), "rb");
                if(!fh)
                {
                    printf("Add mode: file not found: '%s'\n", it->c_str());
                    continue;
                }
                fseek(fh, 0, SEEK_END);
                fpos_t s = ftell(fh);
                fseek(fh, 0, SEEK_SET);
                uint8 *buf = new uint8[s];
                fread(buf, s, 1, fh);
                fclose(fh);
                uint8 flags = LVPAFLAG_PACKED;
                if(solid)
                    flags |= LVPAFLAG_SOLID;
                f.Add((char*)(relPath + *it).c_str(), memblock(buf, s), LVPAFileFlags(flags));
            }
            result = f.Save(level);
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
                result = f.Save(level);
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
    }

    if(errstr.length())
        printf("%s", errstr.c_str());


    f.Clear();
    return result ? 0 : 1;
}

