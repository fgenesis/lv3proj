
#ifndef _LZOCOMPRESSOR_H
#define _LZOCOMPRESSOR_H

#include "ICompressor.h"


class LZOCompressor : public ICompressor
{
public:
    virtual void Compress(uint32 level = 1, ProgressCallback pcb = NULL);
    virtual void Decompress(void);

private:
    static bool s_lzoNeedsInit;
};


#endif
