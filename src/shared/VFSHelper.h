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
    bool AddContainer(LVPAFile *f, bool deleteLater);
    bool AddPath(const char *path);
    void AddVFSDir(VFSDir *dir);
    void Prepare(bool clear = true);
    void Reload(void);
    VFSFile *GetFile(const char *fn);
    VFSDir *GetDir(const char* dn);


protected:
    void _delete(void);
    // the VFSDirs are merged in their declaration order.
    // when merging, files already contained can be overwritten by files merged in later.
    VFSDir *vRoot; // contains all basic files necessary to load other files
    VFSDir *filesysRoot; // local files on disk (root dir)
    std::list<VFSDir*> vlist; // all other files added later
    std::set<LVPAFile*> lvpalist; // LVPA files delayed for deletion, required here
    LVPAFile *lvpabase;

    VFSDir *merged; // contains the merged virtual/actual file system tree
};

#endif
