#ifndef LVPAFILE_H
#define LVPAFILE_H

#include <map>

enum LVPAMasterFlags
{
    LVPAHDR_NONE   = 0x00,
    LVPAHDR_PACKED = 0x01
};

enum LVPAFileFlags
{
    LVPAFLAG_NONE     = 0x00,
    LVPAFLAG_PACKED   = 0x01, // file is packed (LZMA)
    LVPAFLAG_SOLID    = 0x02 // file is in the solid block
};

enum LVPALoadFlags
{
    LVPALOAD_NONE     = 0x00, // load only headers
    LVPALOAD_SOLID    = 0x01, // load the solid block and all files within
    LVPALOAD_ALL      = 0xFF  // load all files
};

enum LVPAAlgorithms
{
    LVPAPACK_LZMA,
    LVPAPACK_LZO1X, // NYI, planned for later

    LVPAPACK_DEFAULT = 0xFF // select the one used for the global block
};

// default compression level used if nothing else is specified [0..9]
#define LVPA_DEFAULT_LEVEL 3

// each buffer allocated for files gets this amount of extra bytes (for pure text files that need to end with \0, for example
#define LVPA_EXTRA_BUFSIZE 4

typedef std::map<std::string, uint32> LVPAIndexMap;

struct LVPAMasterHeader
{
    // char magic[4]; // "LVPA"
    uint32 version;
    uint32 flags; // see LVPAMasterFlags
    uint32 packedHdrSize;
    uint32 realHdrSize; // unpacked size
    uint32 hdrOffset; // absolute offset where the compressed headers start
    uint32 hdrCrc; // checksum for the unpacked headers
    uint32 hdrEntries; // amount of LVPAFileHeaders
    uint32 dataOffs; // absolute offset where the data blocks start
    uint8 algo; // algorithm used to compress the headers
    uint8 packProps; // LZMA props that were used for header compression
};

struct LVPAFileHeader
{
    std::string filename; // in LVPAFileHeaderEx
    uint32 packedSize; // size in bytes in current file (usually packed)
    uint32 realSize; // unpacked size of the file, for array allocation
    uint32 crc; // checksum for the unpacked data block
    uint8 flags; // see LVPAFileFlags
    uint8 algo; // algorithm used to compress this file
    uint8 props; // LZMA props that were used for compression

    // calculated during load, or only required for saving
    uint32 offset; // absolute offset in the file where the data block starts
    memblock data;
    bool good;

};


class LVPAFile
{
public:
    LVPAFile();
    ~LVPAFile();
    bool LoadFrom(const char *fn, LVPALoadFlags loadFlags);
    virtual bool Save(uint32 compression = LVPA_DEFAULT_LEVEL, uint8 algo = LVPAPACK_DEFAULT);
    virtual bool SaveAs(const char *fn, uint32 compression = LVPA_DEFAULT_LEVEL, uint8 algo = LVPAPACK_DEFAULT);

    virtual void Add(const char *fn, memblock mb, LVPAFileFlags flags, uint8 algo = LVPAPACK_DEFAULT); // adds a file, overwriting if exists
    virtual memblock Remove(const char *fn); // removes a file from the container and returns its memblock
    memblock Get(const char *fn);
    void Clear(void); // free all
    virtual bool Delete(const char *fn); // removes a file from the container and frees up memory. returns false if the file was not found.
    void Free(const char *fn); // frees the memory associated with a file, leaving it in the container. if requested again, it will be loaded from disk.
    void Drop(const char *fn); // drops our reference to the file, so it will be loaded again from disk if required.

    inline uint32 Count(void) { return _indexes.size(); }
    inline uint32 HeaderCount(void) { return _headers.size(); }
    bool HasFile(const char *fn);
    const char *GetMyName(void) { return _ownName.c_str(); }

    bool AllGood(void);
    const LVPAFileHeader GetFileInfo(uint32 i);

    // for stats. note: only call these directly after load/save, and NOT after files were added/removed!
    inline uint32 GetRealSize(void) { return _realSize; }
    uint32 GetPackedSize(void) { return _packedSize; }


private:
    std::string _ownName;
    memblock _solidBlock;
    LVPAIndexMap _indexes;
    std::vector<LVPAFileHeader> _headers;
    FILE *_handle;
    uint32 _realSize, _packedSize; // for stats

    memblock _LoadFile(LVPAFileHeader& h);
    bool _OpenFile(void);
    void _CloseFile(void);
    void _CreateIndexes(void);

};

class LVPAFileReadOnly : public LVPAFile
{
    virtual bool Save(uint32 compression = LVPA_DEFAULT_LEVEL, uint8 algo = LVPAPACK_DEFAULT) { return false; }
    virtual bool SaveAs(const char *fn, uint32 compression = LVPA_DEFAULT_LEVEL, uint8 algo = LVPAPACK_DEFAULT) { return false; }
    memblock Remove(char *fn) { return Get(fn); }
    bool Delete(char *fn) { return false; }
    void Add(char *fn, memblock mb, LVPAFileFlags flags) {}
};



#endif

