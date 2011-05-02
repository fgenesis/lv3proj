#include "common.h"
#include "VFSFile.h"
#include "VFSLoader.h"


VFSFile *VFSLoaderDisk::Load(const char *fn)
{
    FILE *fp = fopen(fn, "r");
    if(fp)
    {
        fclose(fp);
        return new VFSFileReal(fn); // must contain full file name
    }
    return NULL;
}
