#include "common.h"
#include "LVPAFile.h"
#include "VFSDir.h"
#include "VFSDirLVPA.h"
#include "VFSFile.h"
#include "VFSHelper.h"
#include "VFSLoader.h"
#include "VFSLoaderLVPA.h"

#define OMNIPRESENT_LOADERS 2
#define LDR_DISK 0
#define LDR_LVPABASE 1

VFSHelper::VFSHelper()
: vRoot(NULL), filesysRoot(NULL), merged(NULL), lvpabase(NULL)
{
    loaders.resize(OMNIPRESENT_LOADERS);
    loaders[LDR_DISK    ] = NULL; // reserved for VFSLoaderReal
    loaders[LDR_LVPABASE] = NULL; // reserved for for VFSLoaderLVPA(lvpabase)
    // other loaders may follow
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
    for(uint32 i = OMNIPRESENT_LOADERS; i < loaders.size(); ++i)
        delete loaders[i];
    loaders.resize(OMNIPRESENT_LOADERS); // drop all except first 2
}

void VFSHelper::LoadBase(LVPAFile *f, bool deleteLater)
{
    if(vRoot)
        vRoot->ref--;
    if(lvpabase)
    {
        delete lvpabase;
        lvpabase = NULL;
    }

    vRoot = new VFSDirLVPA(f);
    vRoot->load();

    if(deleteLater)
        lvpabase = f;

    if(loaders[LDR_LVPABASE])
        delete loaders[LDR_LVPABASE];

    // if the container has scrambled files, register a loader.
    // we can't add scrambled files to the tree, because their names are probably unknown at this point
    for(uint32 i = 0; i < f->HeaderCount(); ++i)
    {
        if(f->GetFileInfo(i).flags & LVPAFLAG_SCRAMBLED)
        {
            loaders[LDR_LVPABASE] = new VFSLoaderLVPA(f);
            break;
        }
    }
}

bool VFSHelper::LoadFileSysRoot(void)
{
    VFSDirReal *oldroot = filesysRoot;

    filesysRoot = new VFSDirReal;
    if(!filesysRoot->load("."))
    {
        filesysRoot->ref--;
        filesysRoot = oldroot;
        return false;
    }

    if(oldroot)
        oldroot->ref--;

    if(!loaders[LDR_DISK])
        loaders[LDR_DISK] = new VFSLoaderDisk;

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

        // if the container has scrambled files, register a loader.
        // we can't add scrambled files to the tree, because their names are probably unknown at this point
        for(uint32 i = 0; i < f->HeaderCount(); ++i)
        {
            if(f->GetFileInfo(i).flags & LVPAFLAG_SCRAMBLED)
            {
                loaders.push_back(new VFSLoaderLVPA(f));
                break;
            }
        }
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
    VFSFile *vf = merged->getFile(fn);

    // nothing found? maybe a loader has something.
    // if so, add the newly created VFSFile to the tree
    if(!vf)
    {
        for(uint32 i = 0; i < loaders.size(); ++i)
        {
            if(loaders[i])
            {
                vf = loaders[i]->Load(fn);
                if(vf)
                {
                    GetDirRoot()->addRecursive(vf, true);
                    --(vf->ref);
                    break;
                }
            }
        }
    }

    return vf;
}

VFSDir *VFSHelper::GetDir(const char* dn, bool create /* = false */)
{
    return *dn ? merged->getDir(dn, create) : merged;
}

VFSDir *VFSHelper::GetDirRoot(void)
{
    return merged;
}
