#ifndef ANIMPARSER_H
#define ANIMPARSER_H

#include "Anim.h"

Anim *ParseAnimData(char *strbuf, char *filename);
Anim *LoadAnimFile(char* fn);


#endif
