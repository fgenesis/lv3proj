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
    uint8 packProps; // LZMA props that were used for header compression
};

struct LVPAFileHeader
{
    std::string filename; // in LVPAFileHeaderEx
    uint32 packedSize; // size in bytes in current file (usually packed)
    uint32 realSize; // unpacked size of the file, for array allocation
    uint32 crc; // checksum for the unpacked data block
    uint8 flags; // see LVPAFileFlags
    uint8 props; // LZMA props that were used for compression

    // calculated during load
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
    bool Save(uint32 compression);
    bool SaveAs(const char *fn, uint32 compression);

    void Add(char *fn, memblock mb, LVPAFileFlags flags); // adds a file, overwriting an existing one
    memblock Remove(char *fn); // removes a file from the container and returns its memblock
    bool Delete(char *fn); // removes a file from the container and frees up memory. returns false if the file was not found.
    memblock Get(char *fn);
    void Clear(void); // free all
    void Free(char *fn);
    

    inline uint32 Count(void) { return _indexes.size(); }
    inline uint32 HeaderCount(void) { return _headers.size(); }
    inline bool HasFile(char *fn) { return Get(fn).ptr; };

    bool AllGood(void);
    const LVPAFileHeader GetFileInfo(uint32 i);


private:
    std::string _ownName;
    memblock _solidBlock;
    LVPAIndexMap _indexes;
    std::vector<LVPAFileHeader> _headers;
    FILE *_handle;

    memblock _LoadFile(LVPAFileHeader& h);
    bool _OpenFile(void);
    void _CloseFile(void);
    void _CreateIndexes(void);

};



#endif

