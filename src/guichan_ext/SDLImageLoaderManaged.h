#ifndef GCN_SDLIMAGELOADER_VFS_HPP
#define GCN_SDLIMAGELOADER_VFS_HPP

#include "guichan/sdl/sdlimageloader.hpp"
#include "guichan/platform.hpp"

#include <SDL/SDL.h>

namespace gcn
{
    class Image;

    /**
    * SDL implementation of ImageLoader.
    */
    class GCN_EXTENSION_DECLSPEC SDLImageLoaderManaged : public SDLImageLoader
    {
        // Inherited from SDLImageLoader
        virtual Image* load(const std::string& filename, bool convertToDisplayFormat = true);

    protected:
        virtual SDL_Surface* loadSDLSurface(const std::string& filename);
    };
}

#endif // end GCN_SDLIMAGELOADER_VFS_HPP
