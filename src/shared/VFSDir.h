#ifndef VFSDIR_H
#define VFSDIR_H

#include <map>
#include "SelfRefCounter.h"

class VFSDir;
class VFSFile;

typedef std::map<std::string, VFSDir*> VFSDirMap;
typedef std::map<std::string, VFSFile*> VFSFileMap;

class VFSDir
{
public:
    VFSDir() : ref(this) {}
    virtual ~VFSDir();
    virtual uint32 load(const char *dir = NULL) { return 0; } // dir must be absolute path
    virtual VFSFile *getFile(const char *fn);
    virtual VFSDir *getDir(const char *subdir, bool forceCreate = false);

    bool insert(VFSDir *subdir, bool overwrite = true);
    bool merge(VFSDir *dir, bool overwrite = true);
    bool add(VFSFile *f, bool overwrite = true);

    const char *name() { return _name.c_str(); }

    VFSFileMap _files;
    VFSDirMap _subdirs;
    std::string _name;

    SelfRefCounter<VFSDir> ref;
};

class VFSDirReal : public VFSDir
{
public:
    VFSDirReal();
    virtual ~VFSDirReal() {};
    virtual uint32 load(const char *dir = NULL);

protected:
    std::string _abspath;
};

#endif
