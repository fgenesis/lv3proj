#ifndef _COMMON_H
#define _COMMON_H

#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

#ifndef SIGQUIT
#define SIGQUIT 3
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <deque>
#include <vector>

#include "SysDefs.h"
#include "DebugStuff.h"
#include "Errors.h"
#include "tools.h"
#include "log.h"

#include "MemoryLeaks.h"


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

struct memblock
{
    memblock() : ptr(NULL), size(0) {}
    memblock(uint8 *p, uint32 s) : size(s), ptr(p) {}
    uint8 *ptr;
    uint32 size;
};

#include "UndefUselessCrap.h"


#endif

