#include "common.h"
#include "LVPAFile.h"
#include "LVPAFileStore.h"

void LVPAFileStore::FreeAll(void)
{
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
        delete *it;
    _fileList.clear();
}

void LVPAFileStore::Add(LVPAFile *f)
{
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
        if(*it == f)
            return;
    _fileList.push_back(f);
}

void LVPAFileStore::AddFront(LVPAFile *f)
{
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
        if(*it == f)
            return;
    _fileList.push_front(f);
}

bool LVPAFileStore::Unlink(const char *name)
{
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
        if(!strcmp((*it)->GetMyName(), name))
        {
            _fileList.erase(it);
            return true;
        }

    return false;
}

void LVPAFileStore::AddAfter(LVPAFile *newf, const char  *name)
{
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
        if(!strcmp((*it)->GetMyName(), name))
        {
            _fileList.insert(it, newf);
            return;
        }
}

LVPAFileStore::File LVPAFileStore::Get(const char  *fn)
{
    File file;
    file.src = NULL;
    for(LVPAFileList::iterator it = _fileList.begin(); it != _fileList.end(); it++)
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
