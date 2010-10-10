#ifndef VFSFILE_H
#define VFSFILE_H

#include "SelfRefCounter.h"

class membuf;

// basic interface
class VFSFile
{
public:
    enum
    {
        npos = 0xFFFFFFFF,
        npos64 = UI64LIT(0xFFFFFFFFFFFFFFFF)
    };
    virtual ~VFSFile() {};
    VFSFile() : ref(this) {}
    virtual bool open(const char *fn = NULL, char *mode = NULL) { return false; }
    virtual bool isopen(void) { return false; }
    virtual bool iseof(void) { return true; }
    virtual const char *name(void) { return ""; }
    virtual const char *fullname(void) { return ""; }
    virtual bool close(void) { return false; }
    virtual bool seek(uint64 pos) { return false; }
    virtual bool seekRel(int64 offs) { return seek(getpos() + offs); }
    virtual bool flush(void) { return false; }
    virtual uint64 getpos(void) { return npos64; }
    virtual uint32 read(char *dst, uint32 bytes) { return npos; }
    virtual uint32 write(char *src, uint32 bytes) { return npos; }
    virtual uint64 size(void) { return 0; }
    virtual uint64 size(uint64 newsize) { return 0; }
    virtual const uint8 *getBuf(void) { return NULL; }
    virtual const char *getSource(void) { return "<BASE>"; }

    SelfRefCounter<VFSFile> ref;
};

class VFSFileReal : public VFSFile
{
public:
    VFSFileReal(const char *name = NULL);
    virtual ~VFSFileReal();
    virtual bool open(const char *fn = NULL, char *mode = NULL);
    virtual bool isopen(void);
    virtual bool iseof(void);
    virtual const char *name(void);
    virtual const char *fullname(void);
    virtual bool close(void);
    virtual bool seek(uint64 pos);
    virtual bool seekRel(int64 offs);
    virtual bool flush(void);
    virtual uint64 getpos(void);
    virtual uint32 read(char *dst, uint32 bytes);
    virtual uint32 write(char *src, uint32 bytes);
    virtual uint64 size(void);
    virtual uint64 size(uint64 newsize);
    virtual const uint8 *getBuf(void);
    virtual const char *getSource(void) { return "disk"; }

protected:
    void _setName(const char *n);
    std::string _fullname;
    std::string _name;
    FILE *_fh;
    uint64 _size;
    uint8 *_buf;
};

#endif
