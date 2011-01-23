#include "common.h"
#include "MyCrc32.h"
#include "LZMACompressor.h"
#include "LVPAFile.h"

const char* gMagic = "LVPA";
const uint32 gVersion = 0;


ByteBuffer &operator >> (ByteBuffer& bb, LVPAMasterHeader& hdr)
{
    bb >> hdr.version;
    bb >> hdr.flags;
    bb >> hdr.hdrEntries;
    bb >> hdr.packedHdrSize;
    bb >> hdr.realHdrSize;
    bb >> hdr.hdrOffset;
    bb >> hdr.hdrCrc;
    bb >> hdr.algo;
    bb >> hdr.packProps;
    bb >> hdr.dataOffs;
    return bb;
}

ByteBuffer &operator << (ByteBuffer& bb, LVPAMasterHeader& hdr)
{
    bb << hdr.version;
    bb << hdr.flags;
    bb << hdr.hdrEntries;
    bb << hdr.packedHdrSize;
    bb << hdr.realHdrSize;
    bb << hdr.hdrOffset;
    bb << hdr.hdrCrc;
    bb << hdr.algo;
    bb << hdr.packProps;
    bb << hdr.dataOffs;
    return bb;
}

ByteBuffer &operator >> (ByteBuffer& bb, LVPAFileHeader& h)
{
    bb >> h.filename;
    bb >> h.packedSize;
    bb >> h.realSize;
    bb >> h.crc;
    bb >> h.flags;
    bb >> h.algo;
    bb >> h.level;
    bb >> h.props;
    return bb;
}

ByteBuffer &operator << (ByteBuffer& bb, LVPAFileHeader& h)
{
    bb << h.filename;
    bb << h.packedSize;
    bb << h.realSize;
    bb << h.crc;
    bb << h.flags;
    //bb << h.algo; // TODO: after implementing other algos, uncomment
    bb << uint8(LVPAPACK_LZMA);
    bb << h.level;
    bb << h.props;
    return bb;
}


LVPAFile::LVPAFile()
: _handle(NULL), _realSize(0), _packedSize(0)
{
}

LVPAFile::~LVPAFile()
{
    Clear();
    _CloseFile();
}

void LVPAFile::Clear(void)
{
    for(uint32 i = 0; i < _headers.size(); ++i)
    {
        if(_headers[i].data.ptr)
        {
            delete [] _headers[i].data.ptr;
            _headers[i].data.ptr = NULL;
            _headers[i].data.size = 0;
        }
    }
    _headers.clear();
    _indexes.clear();
}

bool LVPAFile::_OpenFile(void)
{
    if(!_handle)
        _handle = fopen(_ownName.c_str(), "rb");

    return _handle != NULL;
}

void LVPAFile::_CloseFile(void)
{
    if(_handle)
    {
        fclose(_handle);
        _handle = NULL;
    }
}

void LVPAFile::Add(const char *fn, memblock mb, LVPAFileFlags flags, uint8 algo /* = LVPAPACK_DEFAULT */,
                   uint8 level /* = LVPACOMP_INHERIT */)
{
    LVPAIndexMap::iterator it = _indexes.find(fn);
    if(it == _indexes.end())
    {
        LVPAFileHeader hdr;
        hdr.data = mb;
        hdr.flags = flags;
        hdr.filename = fn;
        hdr.algo = algo;
        hdr.level = level;
        hdr.good = true;
        _headers.push_back(hdr);
        _indexes[fn] = _headers.size() - 1; // save the index of the hdr we just added
    }
    else
    {
        LVPAFileHeader& hdrRef = _headers[it->second];
        if(hdrRef.data.ptr && hdrRef.data.ptr != mb.ptr)
            delete [] hdrRef.data.ptr;
        hdrRef.data = mb;
        hdrRef.flags = flags;
        hdrRef.algo = algo;
        hdrRef.level = level;
        hdrRef.good = true;
    }
}

memblock LVPAFile::Remove(const char  *fn)
{
    LVPAIndexMap::iterator it = _indexes.find(fn);
    if(it == _indexes.end())
        return memblock();

    uint32 idx = it->second;
    memblock mb = _headers[idx].data; // copy ptr
    _headers[idx].data = memblock(); // overwrite with empty
    _indexes.erase(it); // remove entry

    return mb;
}

bool LVPAFile::Delete(const char  *fn)
{
    memblock mb = Remove(fn);
    if(mb.ptr)
    {
        delete [] mb.ptr;
        return true;
    }
    return false;
}

bool LVPAFile::HasFile(const char  *fn) const
{
    LVPAIndexMap::const_iterator it = _indexes.find(fn);
    return it != _indexes.end();
}

memblock LVPAFile::Get(const char *fn)
{
    LVPAIndexMap::iterator it = _indexes.find(fn);
    if(it == _indexes.end())
        return memblock();

    return Get(it->second);
}

memblock LVPAFile::Get(uint32 index)
{
    LVPAFileHeader& hdrRef = _headers[index];
    if(hdrRef.data.ptr) // already loaded, good
        return hdrRef.data;

    // seems we don't have the data yet, load from disk
    return _LoadFile(hdrRef);
}

void LVPAFile::Free(const char  *fn)
{
    LVPAIndexMap::iterator it = _indexes.find(fn);
    if(it == _indexes.end())
        return;

    LVPAFileHeader& hdrRef = _headers[it->second];
    if(hdrRef.data.ptr)
    {
        delete [] hdrRef.data.ptr;
        hdrRef.data.ptr = NULL;
    }
    hdrRef.data.size = 0;
}

void LVPAFile::Drop(const char  *fn)
{
    LVPAIndexMap::iterator it = _indexes.find(fn);
    if(it == _indexes.end())
        return;

    LVPAFileHeader& hdrRef = _headers[it->second];
    hdrRef.data.ptr = NULL;
    hdrRef.data.size = 0;
}

bool LVPAFile::LoadFrom(const char *fn, LVPALoadFlags loadFlags)
{
    _ownName = fn;

    if(!_OpenFile())
        return false;

    Clear();

    uint32 bytes;
    char magic[4];

    bytes = fread(magic, 1, 4, _handle);
    if(bytes != 4 || memcmp(magic, gMagic, 4))
    {
        _CloseFile();
        return false;
    }

    LZMACompressor buf;
    LVPAMasterHeader masterHdr;

    buf.resize(sizeof(LVPAMasterHeader));
    bytes = fread((void*)buf.contents(), 1, sizeof(LVPAMasterHeader), _handle);
    buf >> masterHdr; // not reading it directly via fread() is intentional

    DEBUG(logdebug("master: version: %u", masterHdr.version));
    DEBUG(logdebug("master: data offset: %u", masterHdr.dataOffs));
    DEBUG(logdebug("master: header offset: %u", masterHdr.hdrOffset));
    DEBUG(logdebug("master: header entries: %u", masterHdr.hdrEntries));
    DEBUG(logdebug("master: header crc: %X", masterHdr.hdrCrc));
    DEBUG(logdebug("master: flags: %u", masterHdr.flags));
    DEBUG(logdebug("master: packed size: %u", masterHdr.packedHdrSize));
    DEBUG(logdebug("master: real size: %u", masterHdr.realHdrSize));

    // ... space for additional data/headers here...

    // seek to the file header's offset if we are not yet there
    if(ftell(_handle) != masterHdr.hdrOffset)
        fseek(_handle, masterHdr.hdrOffset, SEEK_SET);

    // read the (packed) file headers
    buf.resize(masterHdr.packedHdrSize);
    bytes = fread((void*)buf.contents(), 1, masterHdr.packedHdrSize, _handle);
    if(bytes != masterHdr.packedHdrSize)
    {
        logerror("Can't read headers, file is corrupt");
        _CloseFile();
        return false;
    }
    
    // decompress the headers if packed
    if(masterHdr.flags & LVPAHDR_PACKED)
    {
        buf.Compressed(true); // tell the buf that it is compressed so it will allow decompression
        buf.RealSize(masterHdr.realHdrSize);
        buf.Decompress(masterHdr.packProps);
    }

    // check CRC
    {
        CRC32 crc;
        crc.Update((uint8*)buf.contents(), buf.size());
        crc.Finalize();
        if(crc.Result() != masterHdr.hdrCrc)
        {
            logerror("CRC mismatch, header is damaged (crc: %X)", crc.Result());
            _CloseFile();
            return false;
        }
    }

    _realSize = _packedSize = 0;

    LVPAFileHeader solidBlockHdr;
    solidBlockHdr.realSize = 0; // we use this as indicator if it is used

    // read the headers
    _headers.resize(masterHdr.hdrEntries);
    uint32 dataStartOffs = masterHdr.dataOffs;
    uint32 solidOffs = 0;
    for(uint32 i = 0; i < masterHdr.hdrEntries; )
    {
        LVPAFileHeader &h = _headers[i];
        buf >> h;

        h.good = true;

        if(h.flags & LVPAFLAG_SOLID)
        {
            h.offset = solidOffs;
            solidOffs += h.packedSize;
            DEBUG(logdebug("(S) '%s' bytes: %u offset: %u", h.filename.c_str(), h.packedSize, h.offset));
            
        }
        else
        {
            h.offset = dataStartOffs;
            dataStartOffs += h.packedSize; // next file starts where this one ended
            DEBUG(logdebug("'%s' bytes: %u offset: %u", h.filename.c_str(), h.packedSize, h.offset));

            // for stats (the solid block will also be covered here)
            _packedSize += h.packedSize;
        }
        _realSize += h.realSize;

        if(h.filename.empty()) // empty filename? this is the solid block then.
        {
            solidBlockHdr = h;
            --masterHdr.hdrEntries;

            // for stats - the solid block size has to be subtracted, because the block itself doesnt count,
            // only the files contained within.
            _realSize -= h.realSize;
        }
        else
            ++i;
    }

    // first, unpack the solid block, if there is one
    if(solidBlockHdr.realSize)
    {
        _headers.resize(masterHdr.hdrEntries); // since the solid block hdr was not stored here, there is 1 space too much, which must be removed
        
        LVPAFileHeader &h = solidBlockHdr;
        if(ftell(_handle) != h.offset)
            fseek(_handle, h.offset, SEEK_SET);

        buf.resize(h.packedSize);
        bytes = fread((void*)buf.contents(), 1, h.packedSize, _handle);
        if(bytes != h.packedSize)
        {
            logerror("Can't read enough bytes for solid block");
            h.good = false;
        }
        else
        {
            if(h.flags & LVPAFLAG_PACKED)
            {
                buf.Compressed(true); // tell the buf that it is compressed so it will allow decompression
                buf.RealSize(h.realSize);
                buf.Decompress(h.props);
            }

            _solidBlock.size = buf.size();
            _solidBlock.ptr = new uint8[buf.size()];
            buf.read(_solidBlock.ptr, buf.size());
            h.good = true;
        }
    }

    // at this point we have processed all headers
    _CreateIndexes();

    // iterate over all files if requested
    if(loadFlags & LVPALOAD_SOLID)
    {
        for(uint32 i = 0; i < masterHdr.hdrEntries; ++i)
        {
            if(_headers[i].flags & LVPAFLAG_SOLID || loadFlags & LVPALOAD_ALL)
            {
                memblock fmb = _LoadFile(_headers[i]);
                if(fmb.ptr)
                {
                    _headers[i].data = fmb;
                    _headers[i].good = true;
                }
                else
                    _headers[i].good = false;
            }
        }

        if(loadFlags & LVPALOAD_ALL)
            _CloseFile(); // got everything, file can be closed
    }
    // otherwise, leave the file open, as we may want to read more later on

    return true;
}

bool LVPAFile::Save(uint8 compression, uint8 algo /* = LVPAPACK_DEFAULT */)
{
    return SaveAs(_ownName.c_str(), compression);
}

bool LVPAFile::SaveAs(const char *fn, uint8 compression /* = LVPA_DEFAULT_LEVEL */, uint8 algo /* = LVPAPACK_DEFAULT */)
{
    // close the file if already open, to allow overwriting
    _CloseFile();

    FILE *outfile = fopen(fn, "wb");
    if(!outfile)
        return false;

    // this is an invalid setting for the solid block
    if(compression == LVPACOMP_INHERIT)
        compression = LVPA_DEFAULT_LEVEL;

    LVPAMasterHeader masterHdr;
    memset(&masterHdr, 0, sizeof(LVPAMasterHeader));

    fwrite(gMagic, 4, 1, outfile);
    fwrite(&masterHdr, sizeof(LVPAMasterHeader), 1, outfile); // this will be overwritten later

    LZMACompressor zhdr;
    LZMACompressor zsolid;
    uint32 solidSize = 0;
    uint32 solidBlockOffset = 0;

    zhdr.reserve((_headers.size() + 1) * (sizeof(LVPAFileHeader) + 20)); // guess size

    _realSize = _packedSize = 0;

    // find out sizes early to prevent re-allocation,
    // and prepare some of the header fields
    for(uint32 i = 0; i < _headers.size(); ++i)
    {
        LVPAFileHeader& h = _headers[i];
        if(!h.good)
            continue;

        if(h.flags & LVPAFLAG_SOLID)
            solidSize += h.data.size;

        h.realSize = h.packedSize = h.data.size;
        CRC32 crc;
        crc.Update(h.data.ptr, h.data.size);
        crc.Finalize();
        h.crc = crc.Result();

        // for stats
        _realSize += h.realSize;
    }
    if(solidSize)
    {
        zsolid.reserve(solidSize);

        // fill the buffer for the solid block with data
        for(LVPAIndexMap::iterator it = _indexes.begin(); it != _indexes.end(); it++)
        {
            LVPAFileHeader& h = _headers[it->second];
            if(!h.good)
                continue;

            if( !(h.flags & LVPAFLAG_SOLID) )
                continue;

            h.offset = zsolid.wpos();
            DEBUG(logdebug("(S) '%s' offset: %u", h.filename.c_str(), h.offset));
            zsolid.append(h.data.ptr, h.data.size);
            h.flags &= ~LVPAFLAG_PACKED; // solid files are never marked as packed, because the solid block itself is already packed
        }

        LVPAFileHeader solidHeader;
        solidHeader.filename = ""; // is defined to be a file with no name

        solidHeader.realSize = zsolid.size();
        {
            CRC32 crc;
            crc.Update((uint8*)zsolid.contents(), zsolid.size());
            crc.Finalize();
            solidHeader.crc = crc.Result();
        }
        if(compression != LVPACOMP_NONE)
            zsolid.Compress(compression);
        solidHeader.flags = zsolid.Compressed() ? LVPAFLAG_PACKED : LVPAFLAG_NONE;
        solidHeader.packedSize = zsolid.size();
        solidHeader.props = zsolid.GetEncodedProps();
        solidHeader.algo = algo;
        solidHeader.level = compression;
        solidHeader.offset = ftell(outfile); // this is directly after magic and master header
        DEBUG(logdebug("solid block offset: %u", solidHeader.offset));
        fwrite(zsolid.contents(), zsolid.size(), 1, outfile);
        zhdr << solidHeader;
        solidHeader.good = true;

        // for stats
        _packedSize += solidHeader.packedSize;

        // for master header
        solidBlockOffset = solidHeader.offset;
    }
    
    // write the other files into the container file, compressed (if the flag is set)
    // headers are mostly prepared already
    for(LVPAIndexMap::iterator it = _indexes.begin(); it != _indexes.end(); it++)
    {
        LVPAFileHeader& h = _headers[it->second];
        if(!h.good)
            continue;

        // solid files were already written to the solid block
        if( !(h.flags & LVPAFLAG_SOLID) )
        {
            h.offset = ftell(outfile);
            uint8 lvl = (h.level == LVPACOMP_INHERIT ? compression : h.level);

            DEBUG(logdebug("'%s' offset: %u level: %u", h.filename.c_str(), h.offset, h.level));

            if(lvl != LVPACOMP_NONE)
            {
                LZMACompressor compr;
                compr.append(h.data.ptr, h.data.size);
                compr.Compress(lvl);
                if(compr.Compressed())
                    h.flags |= LVPAFLAG_PACKED;
                h.packedSize = compr.size();
                h.props = compr.GetEncodedProps();
                fwrite(compr.contents(), compr.size(), 1, outfile);
            }
            else
            {
                h.flags &= ~LVPAFLAG_PACKED; // not packed
                h.props = 0;
                fwrite(h.data.ptr, h.data.size, 1, outfile);
            }

            // for stats
            _packedSize += h.packedSize;
        }

        zhdr << h;
    }

    {
        CRC32 hcrc;
        hcrc.Update((uint8*)zhdr.contents(), zhdr.size());
        hcrc.Finalize();
        masterHdr.hdrCrc = hcrc.Result();
    }
    masterHdr.hdrEntries = _headers.size() + (solidSize ? 1 : 0);
    masterHdr.realHdrSize = zhdr.size();
    if(compression != LVPACOMP_NONE)
        zhdr.Compress(compression);
    masterHdr.algo = algo;
    masterHdr.packProps = zhdr.GetEncodedProps();
    masterHdr.packedHdrSize = zhdr.size();
    masterHdr.flags = zhdr.Compressed() ? LVPAHDR_PACKED : LVPAHDR_NONE;
    masterHdr.version = gVersion;
    masterHdr.hdrOffset = ftell(outfile);
    masterHdr.dataOffs = solidSize ? solidBlockOffset : 4 + sizeof(LVPAMasterHeader); // +4 for "LVPA"
    DEBUG(logdebug("master data offset: %u", masterHdr.dataOffs));
    DEBUG(logdebug("master header offset: %u", masterHdr.hdrOffset));
    fwrite(zhdr.contents(), zhdr.size(), 1, outfile);

    zhdr.clear();
    zhdr << masterHdr;
    fseek(outfile, 4, SEEK_SET); // after "LVPA"
    fwrite(zhdr.contents(), zhdr.size(), 1, outfile);

    fclose(outfile);

    return true;
}


memblock LVPAFile::_LoadFile(const LVPAFileHeader& h)
{
    // h.good is set to false if there was a previous attempt to load the file that failed
    if( !(h.good && _OpenFile()) )
        return memblock();

    uint8 flags = h.flags;

    if((flags & LVPAFLAG_SOLID) && !_solidBlock.ptr)
    {
        logerror("File '%s' is marked as solid, but there is no solid block", h.filename.c_str());
        return memblock();
    }

    if((flags & LVPAFLAG_SOLID) && (flags & LVPAFLAG_PACKED))
    {
        logerror("File '%s' is marked as packed AND solid, not good (ignoring packed flag)", h.filename.c_str());
        flags &= ~LVPAFLAG_PACKED;
    }

    LZMACompressor buf;
    buf.resize(h.packedSize);
    buf.wpos(0);
    buf.rpos(0);
    uint32 bytes;

    if(flags & LVPAFLAG_SOLID)
    {
        if(h.offset + h.packedSize <= _solidBlock.size)
            buf.append(_solidBlock.ptr + h.offset, h.packedSize);
        else
        {
            logerror("Solid file '%s' exceeds solid block length, can't read", h.filename.c_str());
            return memblock();
        }
    }
    else // not solid, read from file
    {
        // seek if necessary
        if(ftell(_handle) != h.offset)
            fseek(_handle, h.offset, SEEK_SET);

        bytes = fread((void*)buf.contents(), 1, h.packedSize, _handle);
        if(bytes != h.packedSize)
        {
            logerror("Unable to read enough data for file '%s'", h.filename.c_str());
            return memblock();
        }
    }

    if(flags & LVPAFLAG_PACKED)
    {
        buf.Compressed(true); // tell the buf that it is compressed so it will allow decompression
        buf.RealSize(h.realSize);
        buf.Decompress(h.props);
    }

    // check CRC32 for that file
    {
        CRC32 crc;
        if(buf.size())
            crc.Update((uint8*)buf.contents(), buf.size());
        crc.Finalize();
        if(crc.Result() != h.crc)
        {
            logerror("CRC mismatch for '%s', file is corrupt", h.filename.c_str());
            return memblock();
        }
    }

    memblock mb(new uint8[buf.size() + LVPA_EXTRA_BUFSIZE], buf.size());
    if(buf.size())
        buf.read(mb.ptr, buf.size());
    memset(mb.ptr + mb.size, 0, LVPA_EXTRA_BUFSIZE); // zero out extra space

    return mb;
}

const LVPAFileHeader& LVPAFile::GetFileInfo(uint32 i) const
{
    return _headers[i];
}

void LVPAFile::_CreateIndexes(void)
{
    for(uint32 i = 0; i < _headers.size(); ++i)
    {
        LVPAFileHeader& h = _headers[i];
        if(h.filename.size())
        {
            _indexes[h.filename] = i;
        }
    }
}

bool LVPAFile::AllGood(void) const
{
    for(uint32 i = 0; i < _headers.size(); ++i)
    if(!_headers[i].good)
        return false;

    return true;
}
