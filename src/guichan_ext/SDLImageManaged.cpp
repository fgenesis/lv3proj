#include "SDLImageManaged.h"
#include "common.h"
#include "ResourceMgr.h"

namespace gcn {

SDLImageManaged::SDLImageManaged(SDL_Surface* surface, bool /*autoFree*/)
: SDLImage(surface, false)
{}

SDLImageManaged::~SDLImageManaged()
{
    resMgr.Drop(getSurface());
}

}
