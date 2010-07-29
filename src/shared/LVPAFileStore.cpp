#include "common.h"
#include "LVPAFile.h"
#include "LVPAFileStore.h"

void LVPAFileStore::FreeAll(void)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
        delete *it;
    _containers.clear();
}

void LVPAFileStore::AddBack(LVPAFile *f)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
        if(*it == f)
            return;
    _containers.push_back(f);
}

void LVPAFileStore::AddFront(LVPAFile *f)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
        if(*it == f)
            return;
    _containers.push_front(f);
}

bool LVPAFileStore::Remove(const char *name)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
        if(!strcmp((*it)->GetMyName(), name))
        {
            _containers.erase(it);
            return true;
        }

    return false;
}

/*
void LVPAFileStore::AddAfter(LVPAFile *newf, const char  *name)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
        if(!strcmp((*it)->GetMyName(), name))
        {
            _containers.insert(it, newf);
            return;
        }
}
*/

LVPAFileStore::File LVPAFileStore::Get(const char  *fn)
{
    File file;
    file.src = NULL;
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
    {
        file.mb = (*it)->Get(fn);
        if(file.mb.ptr)
        {
            file.src = *it;
            break;
        }
    }
    return file;
}


void LVPAFileStore::GetFileList(std::set<std::string>& files)
{
    for(LVPAFileList::iterator it = _containers.begin(); it != _containers.end(); it++)
    {
        LVPAFile *cf = *it;
        for(uint32 i = 0; i < cf->HeaderCount(); i++)
        {
            const LVPAFileHeader& hdr = cf->GetFileInfo(i);
            files.insert(hdr.filename);
        }
    }
}