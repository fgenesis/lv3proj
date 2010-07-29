#ifndef LVPAFILESTORE_H
#define LVPAFILESTORE_H

#include <set>

class LVPAFile;

typedef std::list<LVPAFile*> LVPAFileList;

class LVPAFileStore
{
public:

    struct File
    {
        memblock mb;
        LVPAFile *src;
    };

    bool Remove(const char *name);
    void AddBack(LVPAFile *f); // files at back will be used last
    void AddFront(LVPAFile *f); // files at front will be used first
    //void AddAfter(LVPAFile *newf, const char *name);
    //void AddBefore(LVPAFile *newf, char *name);
    void FreeAll(void);
    File Get(const char *fn); // return a file stored in registered containers, the first occurance is returned
    void LVPAFileStore::GetFileList(std::set<std::string> &files); //

    inline const LVPAFileList& GetContainerList(void) { return _containers; }

private:
    LVPAFileList _containers;
};



#endif
