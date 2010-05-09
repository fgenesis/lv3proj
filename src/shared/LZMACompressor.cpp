#include "LzmaDec.h"
#include "LzmaEnc.h"
#include "common.h"
#include "LZMACompressor.h"

SRes myLzmaProgressDummy(void *, UInt64 , UInt64 )
{
    return SZ_OK;
}

void *myLzmaAlloc(void *, size_t size)
{
    return malloc(size);
}

void myLzmaFree(void *, void *ptr)
{
    if(ptr)
        free(ptr);
}


LZMACompressor::LZMACompressor()
{
    _iscompressed = false;
    _real_size = 0;
}

void LZMACompressor::Compress(uint32 level)
{
    if( _iscompressed || (!size()))
        return;

    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.level = level;

    SizeT oldsize = size();
    SizeT newsize = oldsize / 20 * 21 + (1 << 16); // we allocate 105% of original size for output buffer

    Byte *buf = new Byte[newsize];

    ISzAlloc alloc;
    alloc.Alloc = myLzmaAlloc;
    alloc.Free = myLzmaFree;

    ICompressProgress progress;
    progress.Progress = myLzmaProgressDummy;

    SizeT propsSize = sizeof(CLzmaEncProps);
    uint32 result = LzmaEncode(buf, &newsize, this->contents(), oldsize, &props, &_propsEnc, &propsSize, 0, &progress, &alloc, &alloc);
    if(result != SZ_OK || !newsize || newsize > oldsize)
    {
        delete [] buf;
        return;
    }

    resize(newsize);
    rpos(0);
    wpos(0);
    append(buf,newsize);
    delete [] buf;

    _iscompressed = true;

    _real_size = oldsize;
}

void LZMACompressor::Decompress(uint8 propsEnc)
{
    if( (!_iscompressed) || (!_real_size) || (!size()))
        return;

    uint32 origsize = _real_size;
    int8 result;
    uint8 *target = new uint8[_real_size];
    wpos(0);
    rpos(0);
    SizeT srcLen = this->size();

    ISzAlloc alloc;
    alloc.Alloc = myLzmaAlloc;
    alloc.Free = myLzmaFree;

    ELzmaStatus status;

    result = LzmaDecode(target, (SizeT*)&_real_size, this->contents(), &srcLen, (const Byte*)&propsEnc, sizeof(CLzmaEncProps), LZMA_FINISH_END, &status, &alloc);
    if( result != SZ_OK || origsize != _real_size)
    {
        DEBUG(logerror("LZMACompressor: Decompress error! result=%d cursize=%u origsize=%u realsize=%u\n",result,size(),origsize,_real_size));
        delete [] target;
        return;
    }
    clear();
    append(target, origsize);
    delete [] target;
    _real_size = 0;
    _iscompressed = false;

}

void LZMACompressor::clear(void)
{
    ByteBuffer::clear();
    _real_size = 0;
    _iscompressed = false;
}
    
