#include "common.h"
#include "ResourceMgr.h"
#include "PropParser.h"


void ParsePropData(char *strbuf, char *dn)
{
    std::vector<std::string> lines;
    uint32 cpos; // comment?
    std::string file_name;
    StrSplit(strbuf, "\n\x0a\x0d", lines);
    for(std::vector<std::string>::iterator lin = lines.begin(); lin != lines.end(); lin++)
    {
        if(lin->length() < 3) // must have at least 3 chars per line: 'a=b' is valid, 'a=' is not
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
            logdebug("Prop '%s=%s' for '%s'", key.c_str(), (char*)lin->c_str() + eqpos + 1, file_name.c_str());
        }
    }
}

// loads a .prop file and assigns its content to the ResourceMgr
void LoadPropFile(char* fn) // filename, directory name
{
    std::string dirname = _PathStripLast(fn);
    if(dirname[dirname.length() - 1] != '/')
        dirname += '/';

    memblock *mb = resMgr.LoadTextFile(fn);
    if(!mb)
    {
        logerror("LoadPropFile: Failed to open '%s'", fn);
        return;
    }
    logdebug("LoadPropFile: '%s' (%s)", fn, dirname.c_str());

    ParsePropData((char*)mb->ptr, (char*)dirname.c_str());
    resMgr.Drop(mb);
}
