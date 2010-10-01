#include "common.h"
#include "VFSFile.h"

VFSFileReal::VFSFileReal(const char *name /* = NULL */) : VFSFile()
{
    _buf = NULL;
    _setName(name);
    _fh = NULL;
    _size = VFSFile::npos64;
}

VFSFileReal::~VFSFileReal()
{
    close();
    if(_buf)
    {
        delete [] _buf;
        _buf = NULL;
    }
}

void VFSFileReal::_setName(const char *n)
{
    if(n && *n)
    {
        const char *slashpos = strrchr(n, '/');
        _name = slashpos ? slashpos + 1 : n;
        _fullname = n;
    }
}

bool VFSFileReal::open(const char *fn /* = NULL */, char *mode /* = NULL */)
{
    if(isopen())
        close();

    if(_buf)
    {
        delete [] _buf;
        _buf = NULL;
    }

    _setName(fn);

    _fh = fopen(_fullname.c_str(), mode ? mode : "rb");
    if(!_fh)
        return false;

    fseek(_fh, 0, SEEK_END);
    _size = getpos();
    fseek(_fh, 0, SEEK_SET);

    return true;
}

bool VFSFileReal::isopen(void)
{
    return _fh;
}

bool VFSFileReal::iseof(void)
{
    return !_fh || feof(_fh);
}

const char *VFSFileReal::name(void)
{
    return _name.c_str();
}

bool VFSFileReal::close(void)
{
    if(_fh)
    {
        fclose(_fh);
        _fh = NULL;
    }

    return true;    
}


bool VFSFileReal::seek(uint64 pos)
{
    if(!_fh)
        return false;
    fseek(_fh, pos, SEEK_SET);
    return true;
}

bool VFSFileReal::seekRel(int64 offs)
{
    if(!_fh)
        return false;
    fseek(_fh, offs, SEEK_CUR);
    return true;
}

bool VFSFileReal::flush(void)
{
    if(_fh)
        return false;
    fflush(_fh);
    return true;
}

uint64 VFSFileReal::getpos(void)
{
    if(!_fh)
        return VFSFile::npos64;
    return ftell(_fh);
}

uint32 VFSFileReal::read(char *dst, uint32 bytes)
{
    if(!_fh)
        return VFSFile::npos;
    return fread(dst, 1, bytes, _fh);
}

uint32 VFSFileReal::write(char *src, uint32 bytes)
{
    if(!_fh)
        return VFSFile::npos;
    return fwrite(src, 1, bytes, _fh);
}

uint64 VFSFileReal::size(void)
{
    if(_size != VFSFile::npos64)
        return _size;
    open();
    close();
    // now size is known.
    return _size;
}

const uint8 *VFSFileReal::getBuf(void)
{
    if(_buf)
        return (const uint8 *)_buf;

    bool op = isopen();

    if(!op && !open()) // open with default params if not open
        return NULL;

    uint32 s = uint32(size());
    _buf = new uint8[s + 4]; // a bit extra padding

    if(op)
    {
        uint64 oldpos = getpos();
        seek(0);
        uint32 offs = read((char*)_buf, s);
        memset(_buf + offs, 0, 4);
        seek(oldpos);
    }
    else
    {
        uint32 offs = read((char*)_buf, s);
        memset(_buf + offs, 0, 4);
        close();
    }
    return (const uint8 *)_buf;
}
