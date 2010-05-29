#ifndef GCN_SDLIMAGE_MANAGED_H
#define GCN_SDLIMAGE_MANAGED_H

#include <SDL/SDL.h>

#include <string>

#include "guichan/color.hpp"
#include "guichan/platform.hpp"
#include "guichan/sdl/sdlimage.hpp"

namespace gcn
{
    /**
    * SDL implementation of Image.
    */
    class GCN_EXTENSION_DECLSPEC SDLImageManaged : public SDLImage
    {
    public:
        /**
        * Constructor. Load an image from an SDL surface.
        *
        * NOTE: The functions getPixel and putPixel are only guaranteed to work
        *       before an image has been converted to display format.
        *
        * @param surface the surface from which to load.
        * @param autoFree true if the surface should automatically be deleted.
        */
        SDLImageManaged(SDL_Surface* surface, bool autoFree);

        /**
        * Destructor.
        */
        virtual ~SDLImageManaged();

        virtual void convertToDisplayFormat() {} // already done in ResourceMgr
        virtual void free() {} // we do everything in the destructor
    };
}

#endif // end GCN_SDLIMAGE_MANAGED_HPP
