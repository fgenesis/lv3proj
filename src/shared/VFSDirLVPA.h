#ifndef VFSDIR_LVPA_H
#define VFSDIR_LVPA_H

#include "VFSDir.h"

class LVPAFile;

class VFSDirLVPA : public VFSDir
{
public:
    VFSDirLVPA(LVPAFile *f);
    virtual ~VFSDirLVPA() {};
    virtual uint32 load(const char *dir = NULL);

protected:
    LVPAFile *_lvpa;
    virtual VFSDirLVPA *_getSubdir(const char *);
};

#endif
