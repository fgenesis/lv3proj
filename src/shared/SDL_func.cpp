#include <SDL/SDL.h>
#include "SDL_func.h"

Uint32 SDLfunc_GetSurfaceBytes(SDL_Surface *surface)
{
    return surface->pitch * surface->h;
}

/*
* Return the pixel value at (x, y)
* NOTE: The surface must be locked before calling this!
*/
Uint32 SDLfunc_getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
#     if SDL_BYTEORDER == SDL_BIG_ENDIAN
            return p[0] << 16 | p[1] << 8 | p[2];
#     else
            return p[0] | p[1] << 8 | p[2] << 16;
#     endif

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void SDLfunc_putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
#    if SDL_BYTEORDER == SDL_BIG_ENDIAN
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
#    else
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
#    endif
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

SDL_Surface *CreateEmptySurfaceFrom(SDL_Surface *src)
{
    if(!src)
        return NULL;
    SDL_Surface *dest = SDL_CreateRGBSurface(src->flags, src->w, src->h, src->format->BitsPerPixel,
        src->format->Rmask,  src->format->Gmask,  src->format->Bmask,  src->format->Amask);
    return dest;
}

SDL_Surface *SurfaceFlipH(SDL_Surface *src)
{
    SDL_Surface *dest = CreateEmptySurfaceFrom(src);
    if(!dest)
        return NULL;
    if(SDL_MUSTLOCK(src))
        SDL_LockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_LockSurface(dest);

    for(int y = 0; y < src->h; ++y)
    {
        for(int x = 0; x < src->w; ++x)
        {
            SDLfunc_putpixel(dest, src->w - x - 1, y, SDLfunc_getpixel(src, x, y));
        }
    }

    if(SDL_MUSTLOCK(src))
        SDL_UnlockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_UnlockSurface(dest);

    return dest;
}

SDL_Surface *SurfaceFlipV(SDL_Surface *src)
{
    SDL_Surface *dest = CreateEmptySurfaceFrom(src);
    if(!dest)
        return NULL;
    if(SDL_MUSTLOCK(src))
        SDL_LockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_LockSurface(dest);

    for(int y = 0; y < src->h; ++y)
    {
        for(int x = 0; x < src->w; ++x)
        {
            SDLfunc_putpixel(dest, x, src->h - y - 1, SDLfunc_getpixel(src, x, y));
        }
    }

    if(SDL_MUSTLOCK(src))
        SDL_UnlockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_UnlockSurface(dest);

    return dest;
}

SDL_Surface *SurfaceFlipHV(SDL_Surface *src)
{
    SDL_Surface *dest = CreateEmptySurfaceFrom(src);
    if(!dest)
        return NULL;
    if(SDL_MUSTLOCK(src))
        SDL_LockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_LockSurface(dest);

    for(int y = 0; y < src->h; ++y)
    {
        for(int x = 0; x < src->w; ++x)
        {
            SDLfunc_putpixel(dest, src->w - x - 1, src->h - y - 1, SDLfunc_getpixel(src, x, y));
        }
    }

    if(SDL_MUSTLOCK(src))
        SDL_UnlockSurface(src);
    if(SDL_MUSTLOCK(dest))
        SDL_UnlockSurface(dest);

    return dest;
}



// copied from guichan
void SDLfunc_drawHLine(SDL_Surface *target, int x1, int y, int x2, int r, int g, int b, int a)
{
    Uint32 pixel = SDL_MapRGBA(target->format, r, g, b, a);
    SDLfunc_drawHLine(target, x1, y, x2, pixel);
}

void SDLfunc_drawHLine(SDL_Surface *target, int x1, int y, int x2, Uint32 pixel)
{
    if (x1 > x2)
    {
        x1 ^= x2;
        x2 ^= x1;
        x1 ^= x2;
    }

    int bpp = target->format->BytesPerPixel;
    int a = pixel >> 24; // TODO: is this correct for big endian?

    SDL_LockSurface(target);

    Uint8 *p = (Uint8 *)target->pixels + y * target->pitch + x1 * bpp;

    switch(bpp)
    {
    case 1:
        for (;x1 <= x2; ++x1)
        {
            *(p++) = pixel;
        }
        break;

    case 2:
        {
            Uint16* q = (Uint16*)p;
            for (;x1 <= x2; ++x1)
            {
                *(q++) = pixel;
            }
            break;
        }
    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        {
            for (;x1 <= x2; ++x1)
            {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
                p += 3;
            }
        }
        else
        {
            for (;x1 <= x2; ++x1)
            {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
                p += 3;
            }
        }
        break;

    case 4:  
        {          
            Uint32* q = (Uint32*)p;
            for (;x1 <= x2; ++x1)
            {
                if (a)
                {
                    *q = SDLfunc_Alpha32(pixel,*q,a);
                    q++;
                }
                else
                {
                    *(q++) = pixel;
                }
            }
            break;
        }

    } // end switch

    SDL_UnlockSurface(target);
}

void SDLfunc_drawVLine(SDL_Surface *target, int x, int y1, int y2, int r, int g, int b, int a)
{
    Uint32 pixel = SDL_MapRGBA(target->format, r, g, b, a);
    SDLfunc_drawVLine(target, x, y1, y2, pixel);
}

void SDLfunc_drawVLine(SDL_Surface *target, int x, int y1, int y2, Uint32 pixel)
{
    if (y1 > y2)
    {
        y1 ^= y2;
        y2 ^= y1;
        y1 ^= y2;
    }

    int bpp = target->format->BytesPerPixel;
    int a = pixel >> 24; // TODO: is this correct for big endian?

    SDL_LockSurface(target);

    Uint8 *p = (Uint8 *)target->pixels + y1 * target->pitch + x * bpp;

    switch(bpp)
    {            
    case 1:
        for (;y1 <= y2; ++y1)
        {
            *p = pixel;
            p += target->pitch;
        }
        break;

    case 2:
        for (;y1 <= y2; ++y1)
        {
            *(Uint16*)p = pixel;
            p += target->pitch;
        }
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        {
            for (;y1 <= y2; ++y1)
            {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
                p += target->pitch;
            }
        }
        else
        {
            for (;y1 <= y2; ++y1)
            {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
                p += target->pitch;
            }
        }
        break;

    case 4:
        Uint32* q = (Uint32*)p;
        for (;y1 <= y2; ++y1)
        {
            if (a)
            {
                *(Uint32*)p = SDLfunc_Alpha32(pixel,*q,a);
            }
            else
            {
                *(Uint32*)p = pixel;
            }
            p += target->pitch;
        }
        break;

    } // end switch

    SDL_UnlockSurface(target);
}

void SDLfunc_drawRectangle(SDL_Surface *target, SDL_Rect& rectangle, int r, int g, int b, int a)
{
    Uint32 pixel = SDL_MapRGBA(target->format, r, g, b, a);
    SDLfunc_drawRectangle(target, rectangle, pixel);
}

void SDLfunc_drawRectangle(SDL_Surface *target, SDL_Rect& rectangle, Uint32 pixel)
{
    int x1 = rectangle.x;
    int y1 = rectangle.y;
    if(x1 >= target->w || y1 >= target->h)
        return;
    int y2 = y1 + rectangle.h - 1;
    int x2 = x1 + rectangle.w - 1;
    if(x2 < 0 || y2 < 0)
        return;

    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= target->w) x2 = target->w - 1;
    if(y2 >= target->h) y2 = target->h - 1;


    SDLfunc_drawHLine(target, x1, y1, x2, pixel);
    SDLfunc_drawHLine(target, x1, y2, x2, pixel);

    SDLfunc_drawVLine(target, x1, y1, y2, pixel);
    SDLfunc_drawVLine(target, x2, y1, y2, pixel);
}

void SDLfunc_drawLine(SDL_Surface *target, int x1, int y1, int x2, int y2, int r, int g, int b, int a)
{
    Uint32 pixel = SDL_MapRGBA(target->format, r, g, b, a);
    SDLfunc_drawLine(target, x1, y1, x2, y2, pixel);
}

#include <algorithm> // for min/max
#include "SDL_gfx/SDL_gfxPrimitives.h"

void SDLfunc_drawLine (SDL_Surface* target, int x1, int y1, int x2, int y2, Uint32 c)
{
    lineColor(target, x1, y1, x2, y2, c);

    /*int dx=abs(x2-x1);
    int dy=abs(y2-y1);
    int i;
    int direction=1;
    float m;

    if(x1 > x2)
        std::swap(x1, x2);
    if(y1 > y2)
        std::swap(y1, y2);
    int maxx = std::min<int>(target->w, x2);
    int maxy = std::min<int>(target->h, y2);
    int minx = x1 < 0 ? 0 : x1;
    int miny = y1 < 0 ? 0 : y1;

    if(SDL_MUSTLOCK(target))
        SDL_LockSurface(target);

    if (dx>=dy)
    {
        m = float(y2-y1)/(x2-x1);
        for (i = minx; i != maxx; ++i)
            SDLfunc_putpixel(target, i, int(m*(i-x1)+y1), c);
    }
    else
    {
        m = float(x2-x1)/(y2-y1);
        for (i = miny; i != maxy; ++i)
             SDLfunc_putpixel(target, int(m*(i-y1)+x1), i, c);
    }

    if(SDL_MUSTLOCK(target))
        SDL_UnlockSurface(target);
        */
}
