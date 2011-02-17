
#ifndef _LZMACOMPRESSOR_H
#define _LZMACOMPRESSOR_H

#include "ByteBuffer.h"


class LZMACompressor : public ByteBuffer
{
public:
    typedef int (*ProgressCallback)(void *, uint64 , uint64);

    LZMACompressor();
    void Compress(uint32 level = 1, ProgressCallback pcb = NULL);
    void Decompress(uint8 propsEnc);
    bool Compressed(void) { return _iscompressed; }
    void Compressed(bool b) { _iscompressed = b; }
    uint32 RealSize(void) { return _iscompressed ? _real_size : 0; }
    void RealSize(uint32 realsize) { _real_size = realsize; }
    void clear(void);
    inline uint8 GetEncodedProps(void) { return _propsEnc; }


protected:
    bool _iscompressed;
    uint64 _real_size;
    uint8 _propsEnc;

};


#endif
