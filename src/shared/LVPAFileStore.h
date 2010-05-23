#ifndef LVPAFILESTORE_H
#define LVPAFILESTORE_H

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

    bool Unlink(const char *name);
    void Add(LVPAFile *f);
    void AddFront(LVPAFile *f);
    void AddAfter(LVPAFile *newf, const char *name);
    //void AddBefore(LVPAFile *newf, char *name);
    void FreeAll(void);
    File Get(const char *fn); // return a file stored in the first of the registered containers

    inline const LVPAFileList& GetFileList(void) { return _fileList; }

private:
    LVPAFileList _fileList;
};



#endif
