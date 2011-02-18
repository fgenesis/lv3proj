#ifndef SDL_FUNC_H
#define SDL_FUNC_H

Uint32 SDLfunc_GetSurfaceBytes(SDL_Surface *surface);

Uint32 SDLfunc_getpixel(SDL_Surface *surface, int x, int y);
void SDLfunc_putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

SDL_Surface *CreateEmptySurfaceFrom(SDL_Surface *src);
SDL_Surface *SurfaceFlipH(SDL_Surface *src);
SDL_Surface *SurfaceFlipV(SDL_Surface *src);
SDL_Surface *SurfaceFlipHV(SDL_Surface *src);

inline unsigned int SDLfunc_Alpha32(unsigned int src, unsigned int dst, unsigned char a)
{
    unsigned int b = ((src & 0xff) * a + (dst & 0xff) * (255 - a)) >> 8;
    unsigned int g = ((src & 0xff00) * a + (dst & 0xff00) * (255 - a)) >> 8;
    unsigned int r = ((src & 0xff0000) * a + (dst & 0xff0000) * (255 - a)) >> 8;

    return (b & 0xff) | (g & 0xff00) | (r & 0xff0000);
}

void SDLfunc_drawRectangle(SDL_Surface *target, SDL_Rect& rectangle, int r, int g, int b, int a);
void SDLfunc_drawRectangle(SDL_Surface *target, SDL_Rect& rectangle, Uint32 pixel);
void SDLfunc_drawVLine(SDL_Surface *target, int x, int y1, int y2, int r, int g, int b, int a);
void SDLfunc_drawVLine(SDL_Surface *target, int x, int y1, int y2, Uint32 pixel);
void SDLfunc_drawHLine(SDL_Surface *target, int x1, int y, int x2, int r, int g, int b, int a);
void SDLfunc_drawHLine(SDL_Surface *target, int x1, int y, int x2, Uint32 pixel);


#endif
