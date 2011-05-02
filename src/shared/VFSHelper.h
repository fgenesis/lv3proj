#ifndef VFSHELPER_H
#define VFSHELPER_H

#include <set>

class VFSDir;
class VFSDirReal;
class VFSDirLVPA;
class VFSFile;
class LVPAFile;
class VFSLoader;

class VFSHelper
{
public:
    VFSHelper();
    ~VFSHelper();
    void LoadBase(LVPAFile *f, bool deleteLater);
    bool LoadFileSysRoot(void);
    bool AddContainer(LVPAFile *f, const char *subdir, bool deleteLater, bool overwrite = true);
    bool AddPath(const char *path);
    void AddVFSDir(VFSDir *dir, const char *subdir = NULL, bool overwrite = true);
    void Prepare(bool clear = true);
    void Reload(bool fromDisk = false);
    VFSFile *GetFile(const char *fn);
    VFSDir *GetDir(const char* dn, bool create = false);
    VFSDir *GetDirRoot(void);

protected:

    struct VDirEntry
    {
        VDirEntry() : vdir(NULL), overwrite(false) {}
        VDirEntry(VFSDir *v, std::string mp, bool ow) : vdir(v), mountPoint(mp), overwrite(ow) {}
        VFSDir *vdir;
        std::string mountPoint;
        bool overwrite;
    };

    typedef std::list<VDirEntry> VFSMountList;

    void _delete(void);
    // the VFSDirs are merged in their declaration order.
    // when merging, files already contained can be overwritten by files merged in later.
    VFSDirLVPA *vRoot; // contains all files from lvpabase
    VFSDirReal *filesysRoot; // local files on disk (root dir)
    VFSMountList vlist; // all other files added later, together with path to mount to
    std::set<LVPAFile*> lvpalist; // LVPA files delayed for deletion, required here
    LVPAFile *lvpabase; // base LVPA file, to be checked if everything else fails
    std::vector<VFSLoader*> loaders; // if files are not in the tree, maybe one of these is able to find it

    VFSDir *merged; // contains the merged virtual/actual file system tree
};

#endif
