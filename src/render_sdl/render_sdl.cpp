#if defined(_MSC_VER) || defined(_WIN32)
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT
#endif

#include <SDL/SDL_video.h>
#include <SDL/SDL_image.h>

#define MAXSTATES 16

static SDL_Surface *target = NULL;
static SDL_Surface * const video = NULL;

struct RenderState
{
    float x, y;
};

static RenderState states[MAXSTATES];
static RenderState *sp = NULL;
static int col = 0;


static bool render_init(void)
{
    sp = &states[0];
    memset(sp, 0, sizeof(states));

    // TODO
    return true;
}

static bool render_close(void)
{
    // TODO
    return true;
}

static bool render_setMode(int w, int h, bool full)
{
    // TODO
    return true;
}

static void render_pushState(void)
{
    *sp = sp[1];
    ++sp;
}
static void render_popState(void)
{
    --sp;
}

static void render_initState(void)
{
    memset(sp, 0, sizeof(RenderState));
}

static void render_colori(int c)
{
    col = c;
}

static void render_color4c(char r, char g, char b, char a)
{
    render_colori((a << 24) | (r << 16) | (g << 8) | b);
}

static void render_color4f(float r, float g, float b, float a)
{
    if(r < 0) r = 0;
    if(r > 1) r = 1;
    if(g < 0) g = 0;
    if(g > 1) g = 1;
    if(b < 0) b = 0;
    if(b > 1) b = 1;
    if(a < 0) a = 0;
    if(a > 1) a = 1;
    render_color4c(char(r * 255), char(g * 255), char(b * 255), char(a * 255));
}

static void render_setTarget(SDL_Surface * h)
{
    target = h ? h : video;
}

static void render_translate(float x, float y)
{
    sp->x += x;
    sp->y += y;
}

static void render_pixel(float x, float y)
{
}

static void render_line(float x1, float y1, float x2, float y2)
{
}

static void render_tex(SDL_Surface * h, float x, float y)
{
}

static int tex_width(SDL_Surface * h)
{
    return h->w;
}

static int tex_height(SDL_Surface * h)
{
    return h->h;
}

static SDL_Surface * tex_create(void *data, size_t size)
{
    SDL_Surface *s = NULL;
    SDL_RWops *rwop = SDL_RWFromMem(data, (int)size);
    if(rwop)
    {
        s = IMG_Load_RW(rwop, 0);
        SDL_RWclose(rwop);
    }
    return s;
}

static SDL_Surface * tex_createRaw(int w, int h, int components, void *pixels)
{
    if(components == 3)
        return SDL_CreateRGBSurfaceFrom(pixels, w, h, 24, w * 3, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
    else if(components == 4)
        return SDL_CreateRGBSurfaceFrom(pixels, w, h, 32, w * 4, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    return NULL;
}

static void tex_free(SDL_Surface * h)
{
    SDL_FreeSurface(h);
}

extern "C"
{

DLL_EXPORT void *get_render_ptrs(void)
{
    static void* ptrs[] =
    {
        (render_init),
        (render_close),
        (render_setMode),

        (render_pushState),
        (render_popState),
        (render_initState),

        (render_color4f),
        (render_color4c),
        (render_colori),

        (render_setTarget),
        (render_translate),
        (render_pixel),
        (render_line),
        (render_tex),

        (tex_width),
        (tex_height),
        (tex_create),
        (tex_createRaw),
        (tex_free)
    };

    return ptrs;
}

}
