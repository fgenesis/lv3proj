#ifndef ASCIILEVELPARSER_H
#define ASCIILEVELPARSER_H

#include "array2d.h"

struct AsciiLevel
{
    array2d<uint8> tiles;
    std::vector<std::string> tiledata[256];
};

AsciiLevel *LoadAsciiLevel(char *fn);
AsciiLevel *ParseAsciiLevel(char *strbuf);


#endif
