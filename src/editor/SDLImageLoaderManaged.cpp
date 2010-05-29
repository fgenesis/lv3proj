#include "guichan/sdl/sdlimage.hpp"
#include <SDL/SDL_image.h>
#include "guichan/exception.hpp"

#include "common.h"
#include "SDLImageLoaderManaged.h"
#include "SDLImageManaged.h"
#include "ResourceMgr.h"

namespace gcn {

SDL_Surface* SDLImageLoaderManaged::loadSDLSurface(const std::string& filename)
{
    return resMgr.LoadImg((char*)filename.c_str());
}

Image* SDLImageLoaderManaged::load(const std::string& filename,
                            bool convertToDisplayFormat)
{
    SDL_Surface *loadedSurface = loadSDLSurface(filename);

    if (loadedSurface == NULL)
    {
        throw GCN_EXCEPTION(
            std::string("Unable to load image file: ") + filename);
    }

    Image *image = new SDLImageManaged(loadedSurface, true);

    return image;
}

}
