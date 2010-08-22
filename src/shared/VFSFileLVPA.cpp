#include "common.h"
#include "LVPAFile.h"
#include "VFSFileLVPA.h"

VFSFileLVPA::VFSFileLVPA(LVPAFile *src, uint32 headerId) : VFSFile()
{
    _lvpa = src;
    _pos = 0;
    const LVPAFileHeader& hdr = src->GetFileInfo(headerId);
    _size = hdr.realSize;
    _headerId = headerId;

    size_t slashpos = hdr.filename.find_last_of('/');
    _name = slashpos != std::string::npos ? hdr.filename.c_str() + slashpos + 1 : hdr.filename;
}

VFSFileLVPA::~VFSFileLVPA()
{
}

bool VFSFileLVPA::open(const char *fn /* = NULL */, char *mode /* = NULL */)
{
    _pos = 0;
    return true; // does not have to be opened
}

bool VFSFileLVPA::isopen(void)
{
    return true; // is always open
}

bool VFSFileLVPA::iseof(void)
{
    return _pos >= _size;
}

const char *VFSFileLVPA::name(void)
{
    return _name.c_str();
}

bool VFSFileLVPA::close(void)
{
    return true; // always open, so this doesn't matter
}

bool VFSFileLVPA::seek(uint64 pos)
{
    if(pos > 0xFFFFFFFF) // LVPA files have uint32 range only
        return false;

    _pos = (uint32)pos;
    return true;
}

bool VFSFileLVPA::flush(void)
{
    return true;
}

uint64 VFSFileLVPA::getpos(void)
{
    return _pos;
}

uint32 VFSFileLVPA::read(char *dst, uint32 bytes)
{
    memblock data = _lvpa->Get(_headerId);
    uint8 *startptr = data.ptr + _pos;
    uint8 *endptr = data.ptr + data.size;
    bytes = std::min(uint32(endptr - startptr), bytes); // limit in case reading over buffer size
    memcpy(dst, startptr, bytes);
    _pos += bytes;
    return bytes;
}

uint32 VFSFileLVPA::write(char *src, uint32 bytes)
{
    return VFSFile::npos; // TODO: implement if necessary
}

uint64 VFSFileLVPA::size(void)
{
    return _size;
}

const uint8 *VFSFileLVPA::getBuf(void)
{
    return (const uint8 *)_lvpa->Get(_headerId).ptr;
}
