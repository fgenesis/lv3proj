#include "common.h"
#include "ResourceMgr.h"
#include "PropParser.h"


void ParsePropData(char *strbuf, char *dn)
{
    std::vector<std::string> lines;
    uint32 cpos; // comment?
    std::string file_name;
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

        // check if its the file name line - [string]
        if((*lin)[0] == '[')
        {
            file_name = lin->c_str() + 1; // skip '['
            uint32 alen = file_name.length();
            if(file_name[alen - 1] == ']') // remove trailing ']'
                file_name.erase(alen - 1);
            file_name = AddPathIfNecessary(file_name, dn);
            continue;
        }

        uint32 eqpos = lin->find('=');
        if(eqpos != std::string::npos && eqpos + 1 < lin->length())
        {
            std::string key = lin->substr(0, eqpos);
            resMgr.SetPropForFile((char*)file_name.c_str(), (char*)key.c_str(), (char*)lin->c_str() + eqpos + 1);
        }
    }
}

// loads a .prop file and assigns its content to the ResourceMgr
void LoadPropFile(char* fn, char *dn)
{
    // TODO: clean this mess up a little
    uint32 dn_len = dn ? strlen(dn) : 0;
    bool endslash = dn[dn_len - 1] == '/';
    std::string fixdn(dn);
    if(!endslash)
        fixdn += '/';
    std::string fullfn(fixdn);
    fullfn += fn;

    FILE *fh = fopen(fullfn.c_str(), "r");
    if(!fh)
    {
        logerror("LoadPropFile: Failed to open '%s'", fn);
        return;
    }

    fseek(fh, 0, SEEK_END);
    uint32 size = ftell(fh);
    rewind(fh);

    char *buf = new char[size];
    uint32 bytes = fread(buf, 1, size, fh);
    ASSERT(bytes <= size);
    buf[bytes] = 0;
    fclose(fh);

    ParsePropData(buf, (char*)fixdn.c_str());
    delete [] buf;
}
