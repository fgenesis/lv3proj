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
    virtual void dropBuf(bool del) {}
    virtual const char *getSource(void) { return "<BASE>"; }

    SelfRefCounter<VFSFile> ref;

private:
    MLD;
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
    virtual void dropBuf(bool del);
    virtual const char *getSource(void) { return "disk"; }

protected:
    void _setName(const char *n);
    std::string _fullname;
    std::string _name;
    FILE *_fh;
    uint64 _size;
    uint8 *_buf;
};

class VFSFileMem : public VFSFile
{
public:
    VFSFileMem(const char *name, uint8 *buf, uint32 size, bool copy = true);
    virtual ~VFSFileMem();
    virtual bool open(const char *fn = NULL, char *mode = NULL) { return true; }
    virtual bool isopen(void) { return true; } // always open
    virtual bool iseof(void) { return _pos >= _size; }
    virtual const char *name(void) { return _name.c_str(); }
    virtual const char *fullname(void) { return _fullname.c_str(); }
    virtual bool close(void) { return false; } // cant close
    virtual bool seek(uint64 pos) { _pos = pos; return true; }
    virtual bool seekRel(int64 offs) { _pos += offs; return true; }
    virtual bool flush(void) { return false; }
    virtual uint64 getpos(void) { return _pos; }
    virtual uint32 read(char *dst, uint32 bytes);
    virtual uint32 write(char *src, uint32 bytes);
    virtual uint64 size(void) { return _size; }
    virtual uint64 size(uint64 newsize) { return _size; } // cant change size
    virtual const uint8 *getBuf(void) { return _buf; }
    virtual void dropBuf(bool) {} // we can't simply drop the internal buffer, as the file is entirely memory based
    virtual const char *getSource(void) { return "mem"; }

protected:
    void _setName(const char *n);
    std::string _fullname;
    std::string _name;
    uint32 _pos;
    uint32 _size;
    uint32 _mybuf;
    uint8 *_buf;
};

#endif
