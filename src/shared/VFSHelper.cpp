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
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); it++)
        it->vdir->ref--;
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
    VFSDir *oldroot = filesysRoot;

    filesysRoot = new VFSDirReal;
    if(!filesysRoot->load("."))
    {
        filesysRoot->ref--;
        filesysRoot = oldroot;
        return false;
    }

    if(oldroot)
        oldroot->ref--;

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

void VFSHelper::Reload(bool fromDisk /* = false */)
{
    if(fromDisk)
        LoadFileSysRoot();
    Prepare(false);
    for(VFSMountList::iterator it = vlist.begin(); it != vlist.end(); it++)
        GetDir(it->mountPoint.c_str(), true)->merge(it->vdir, it->overwrite);
}

void VFSHelper::AddVFSDir(VFSDir *dir, const char *subdir /* = NULL */, bool overwrite /* = true */)
{
    if(!subdir)
        subdir = "";
    dir->ref++;
    VDirEntry ve(dir, subdir, overwrite);
    vlist.push_back(ve);
    GetDir(subdir, true)->merge(dir, overwrite); // merge into specified subdir. will be (virtually) created if not existing
}

bool VFSHelper::AddContainer(LVPAFile *f, const char *path, bool deleteLater, bool overwrite /* = true */)
{
    VFSDirLVPA *vfs = new VFSDirLVPA(f);
    if(vfs->load())
    {
        AddVFSDir(vfs, path, overwrite);
        if(deleteLater)
            lvpalist.insert(f);
    }
    else if(deleteLater)
        delete f; // loading unsucessful, delete now

    return --(vfs->ref); // 0 if if deleted
}

bool VFSHelper::AddPath(const char *path)
{
    VFSDirReal *vfs = new VFSDirReal;
    if(vfs->load(path))
        AddVFSDir(vfs);
    return --(vfs->ref); // 0 if deleted
}

VFSFile *VFSHelper::GetFile(const char *fn)
{
    return merged->getFile(fn);
}

VFSDir *VFSHelper::GetDir(const char* dn, bool create /* = false */)
{
    return *dn ? merged->getDir(dn, create) : merged;
}

VFSDir *VFSHelper::GetDirRoot(void)
{
    return merged;
}
