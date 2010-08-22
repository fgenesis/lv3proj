#include "common.h"
#include "LVPAFile.h"
#include "VFSDir.h"
#include "VFSDirLVPA.h"
#include "VFSFile.h"
#include "VFSHelper.h"

VFSHelper::VFSHelper()
: vRoot(NULL), filesysRoot(NULL), merged(NULL), lvpabase(NULL)
{
}

VFSHelper::~VFSHelper()
{
    _delete();
    if(vRoot)
        vRoot->ref--;
    if(filesysRoot)
        filesysRoot->ref--;
    if(lvpabase)
        delete lvpabase;
}

void VFSHelper::_delete(void)
{
    if(merged)
    {
        merged->ref--;
        merged = NULL;
    }
    for(std::list<VFSDir*>::iterator it = vlist.begin(); it != vlist.end(); it++)
        (*it)->ref--;
    for(std::set<LVPAFile*>::iterator it = lvpalist.begin(); it != lvpalist.end(); it++)
        delete *it;
    vlist.clear();
    lvpalist.clear();
}

bool VFSHelper::LoadBase(LVPAFile *f, bool deleteLater)
{
    if(vRoot)
        vRoot->ref--;
    if(lvpabase)
    {
        delete lvpabase;
        lvpabase = NULL;
    }

    vRoot = new VFSDirLVPA(f);
    if(!vRoot->load())
    {
        vRoot->ref--;
        vRoot = NULL;
        return false;
    }
    if(deleteLater)
        lvpabase = f;

    return true;
}

bool VFSHelper::LoadFileSysRoot(void)
{
    if(filesysRoot)
        filesysRoot->ref--;

    filesysRoot = new VFSDirReal;
    if(!filesysRoot->load("."))
    {
        filesysRoot->ref--;
        filesysRoot = NULL;
        return false;
    }

    return true;
}

void VFSHelper::Prepare(bool clear /* = true */)
{
    if(clear)
        _delete();
    if(!merged)
        merged = new VFSDir;
    
    if(vRoot)
        merged->merge(vRoot);
    if(filesysRoot)
        merged->merge(filesysRoot);
}

void VFSHelper::Reload(void)
{
    Prepare(false);
    for(std::list<VFSDir*>::iterator it = vlist.begin(); it != vlist.end(); it++)
        merged->merge(*it);
}

void VFSHelper::AddVFSDir(VFSDir *dir)
{
    dir->ref++;
    vlist.push_back(dir);
    merged->merge(dir, true);
}

bool VFSHelper::AddContainer(LVPAFile *f, bool deleteLater)
{
    VFSDirLVPA *vfs = new VFSDirLVPA(f);
    if(vfs->load())
    {
        AddVFSDir(vfs);
        if(deleteLater)
            lvpalist.insert(f);
    }
    else if(deleteLater)
        delete f; // loading unsucessful, delete now

    return vfs->ref--; // 0 if if deleted
}

bool VFSHelper::AddPath(const char *path)
{
    VFSDirReal *vfs = new VFSDirReal;
    if(vfs->load(path))
        AddVFSDir(vfs);
    return vfs->ref--; // 0 if deleted
}

VFSFile *VFSHelper::GetFile(const char *fn)
{
    return merged->getFile(fn);
}

VFSDir *VFSHelper::GetDir(const char* dn)
{
    return merged->getDir(dn);
}
