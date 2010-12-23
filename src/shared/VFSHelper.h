#ifndef VFSHELPER_H
#define VFSHELPER_H

#include <set>

class VFSDir;
class VFSFile;
class LVPAFile;

class VFSHelper
{
public:
    VFSHelper();
    ~VFSHelper();
    bool LoadBase(LVPAFile *f, bool deleteLater);
    bool LoadFileSysRoot(void);
    bool AddContainer(LVPAFile *f, const char *subdir, bool deleteLater);
    bool AddPath(const char *path);
    void AddVFSDir(VFSDir *dir, const char *subdir = NULL);
    void Prepare(bool clear = true);
    void Reload(bool fromDisk = false);
    VFSFile *GetFile(const char *fn);
    VFSDir *GetDir(const char* dn, bool create = false);
    VFSDir *GetDirRoot(void);

    typedef std::list<std::pair<VFSDir*, std::string> > VFSMountList;


protected:
    void _delete(void);
    // the VFSDirs are merged in their declaration order.
    // when merging, files already contained can be overwritten by files merged in later.
    VFSDir *vRoot; // contains all basic files necessary to load other files
    VFSDir *filesysRoot; // local files on disk (root dir)
    VFSMountList vlist; // all other files added later, together with path to mount to
    std::set<LVPAFile*> lvpalist; // LVPA files delayed for deletion, required here
    LVPAFile *lvpabase;

    VFSDir *merged; // contains the merged virtual/actual file system tree
};

#endif
