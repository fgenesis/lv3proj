#ifndef VFSLOADER_H
#define VFSLOADER_H

class VFSFile;

// VFSLoader - to be called if a file is not in the tree.
class VFSLoader
{
public:
    virtual ~VFSLoader() {}
    virtual VFSFile *Load(const char *fn) = 0;
};

class VFSLoaderDisk : public VFSLoader
{
public:
    virtual ~VFSLoaderDisk() {}
    virtual VFSFile *Load(const char *fn);
};


#endif