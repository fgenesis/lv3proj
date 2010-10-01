#include <map>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "common.h"
#include "IMG_savepng.h"

Uint32 getpixel(SDL_Surface *surface, int x, int y)
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
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;

        case 4:
            return *(Uint32 *)p;

        default:
            return 0;       /* shouldn't happen, but avoids warnings */
    }
}

// returns true if both surfaces contain the same pixel data
bool surface_compare(SDL_Surface *a, SDL_Surface *b)
{
    if(a->h != b->h || a->w != b->w)
        return false;


    for(int y = 0; y < a->h; ++y)
        for(int x = 0; x < a->w; ++x)
            if(getpixel(a,x,y) != getpixel(b,x,y))
                return false;

    return true;
}

std::map<std::string, SDL_Surface*> tilemap;

bool addUniqueTile(int x, int y, SDL_Surface *s, char *dirname)
{
    for(std::map<std::string, SDL_Surface*>::iterator it = tilemap.begin(); it != tilemap.end(); it++)
    {
        if(surface_compare(s, it->second))
        {
            printf("(%u, %u) duplicate (with %s).\n", x,y, it->first.c_str());
            return false;
        }
    }

    char fn[1024];
    sprintf(fn, "%s/%u_%u.png",dirname,y,x);
    if(IMG_SavePNG(fn, s, 9) < 0)
    {
        printf("(%u, %u) SAVE ERROR.\n", x,y);
        return false;
    }
    else
    {
        printf("(%u, %u) saved (%s).\n", x,y, fn);
        tilemap[fn] = s;
    }

    return true;
}



int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    atexit(SDL_Quit);

    if(argc < 2) // ---DEBUG---
    {
        printf("Usage: tilegrab <input-file>\nOn windows, just drag a file onto the exe\n");
        return 2;
    }

    char *file = argv[1];

    uint32 xdim = 16, ydim = 16;

    if(argc > 2)
    {
        xdim = atoi(argv[2]);
        if(argc > 3)
            ydim = atoi(argv[3]);
        else
            ydim = xdim;
    }

    printf("Opening '%s'", file);

    SDL_Surface *image = IMG_Load(file);
    if(!image)
        return 1;

    std::string dirname(file);
    dirname += "_export";
    CreateDir(dirname.c_str());

    IMG_SavePNG((char*)(dirname + "/source.png").c_str(), image, 9);

    int maxx = image->w / xdim;
    int maxy = image->h / ydim;

    SDL_Rect rect;
    rect.w = xdim;
    rect.h = ydim;

    //uint8 oalpha = origin->format->alpha;
    //uint8 oflags = origin->flags;
    SDL_SetAlpha(image, 0, 0);

    for(int y = 0; y < maxy; ++y)
    {
        rect.y = y * ydim;
        for(int x = 0; x < maxx; ++x)
        {
            SDL_Surface *tile = SDL_CreateRGBSurface(image->flags, xdim, ydim, 32,
                0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000); // TODO: fix this for big endian
            rect.x = x * xdim;
            SDL_BlitSurface(image, &rect, tile, NULL);
            if(!addUniqueTile(x,y,tile,(char*)dirname.c_str()))
                SDL_FreeSurface(tile);
        }
    }

    for(std::map<std::string, SDL_Surface*>::iterator it = tilemap.begin(); it != tilemap.end(); it++)
    {
        SDL_FreeSurface(it->second);
    }


    return 0;
}
