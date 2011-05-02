#ifndef VFSLOADER_LVPA_H
#define VFSLOADER_LVPA_H

#include "VFSLoader.h"

class LVPAFile;

class VFSLoaderLVPA : public VFSLoader
{
public:
    VFSLoaderLVPA(LVPAFile *lvpa);
    virtual ~VFSLoaderLVPA() {}
    virtual VFSFile *Load(const char *fn);

protected:
    LVPAFile *_lvpa;
};


#endif