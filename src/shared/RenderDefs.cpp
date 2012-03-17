#include "common.h"
#include "RenderDefs.h"

bool (*render_init)(void) = NULL;
bool (*render_close)(void) = NULL;
bool (*render_setMode)(int w, int h, bool full) = NULL;

void (*render_pushState)(void) = NULL;
void (*render_popState)(void) = NULL;
void (*render_initState)(void) = NULL;

void (*render_color4f)(float r, float g, float b, float a) = NULL;
void (*render_color4c)(char r, char g, char b, char a) = NULL;
void (*render_colori)(int c) = NULL;

void (*render_setTarget)(TextureHandle h) = NULL;
void (*render_translate)(float x, float y) = NULL;
void (*render_pixel)(float x, float y) = NULL;
void (*render_line)(float x1, float y1, float x2, float y2) = NULL;
void (*render_tex)(TextureHandle h, float x, float y) = NULL;

int (*tex_width)(TextureHandle h) = NULL;
int (*tex_height)(TextureHandle h) = NULL;
TextureHandle (*tex_create)(void *data, size_t size) = NULL;
TextureHandle (*tex_createRaw)(int w, int h, int components, void *pixels) = NULL;
void (*tex_free)(TextureHandle h) = NULL;

template <typename T> inline static void setptr(T *& dst, void *src)
{
    dst = (T*)src;
}



bool initRenderBackend(void *ptr)
{
    void **p = (void**)ptr;

#define LOAD(x) setptr(x, *p++)
    LOAD(render_init);
    LOAD(render_close);
    LOAD(render_setMode);

    LOAD(render_pushState);
    LOAD(render_popState);
    LOAD(render_initState);

    LOAD(render_color4f);
    LOAD(render_color4c);
    LOAD(render_colori);

    LOAD(render_setTarget);
    LOAD(render_translate);
    LOAD(render_pixel);
    LOAD(render_line);
    LOAD(render_tex);

    LOAD(tex_width);
    LOAD(tex_height);
    LOAD(tex_create);
    LOAD(tex_createRaw);
    LOAD(tex_free);
#undef LOAD
    return true;
}
