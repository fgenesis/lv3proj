#ifndef VFSFILE_LVPA_H
#define VFSFILE_LVPA_H

#include "VFSFile.h"

class LVPAFile;

class VFSFileLVPA : public VFSFile
{
public:
    VFSFileLVPA(LVPAFile *src, uint32 headerId);
    virtual ~VFSFileLVPA();
    virtual bool open(const char *fn = NULL, char *mode = NULL);
    virtual bool isopen(void);
    virtual bool iseof(void);
    virtual const char *name(void);
    virtual const char *fullname(void);
    virtual bool close(void);
    virtual bool seek(uint64 pos);
    virtual bool flush(void);
    virtual uint64 getpos(void);
    virtual uint32 read(char *dst, uint32 bytes);
    virtual uint32 write(char *src, uint32 bytes);
    virtual uint64 size(void);
    virtual uint64 size(uint64 newsize);
    virtual const uint8 *getBuf(void);
    virtual void dropBuf(bool del);
    virtual const char *getSource(void) { return "LVPA"; }

    inline LVPAFile *getLVPA(void) { return _lvpa; }

protected:
    uint32 _pos;
    uint32 _size;
    uint32 _headerId;
    std::string _name;
    std::string _fullname;
    std::string _mode;
    LVPAFile *_lvpa;
    char *_fixedStr; // for \n fixed string in text mode
};

#endif
