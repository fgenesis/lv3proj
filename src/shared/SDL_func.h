#ifndef SDL_FUNC_H
#define SDL_FUNC_H

Uint32 SDLfunc_getpixel(SDL_Surface *surface, int x, int y);
void SDLfunc_putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

SDL_Surface *CreateEmptySurfaceFrom(SDL_Surface *src);
SDL_Surface *SurfaceFlipH(SDL_Surface *src);
SDL_Surface *SurfaceFlipV(SDL_Surface *src);
SDL_Surface *SurfaceFlipHV(SDL_Surface *src);

#endif
