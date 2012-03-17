#ifndef RENDERDEFS_H
#define RENDERDEFS_H

struct SDL_Surface;

struct TextureHandle
{
    union
    {
        void *ptr;
        intptr_t id;
    };

    TextureHandle() {}
    TextureHandle(intptr_t i) : id(i) {}
    TextureHandle(void *p) : ptr(p) {}

    operator SDL_Surface* () { return (SDL_Surface*)ptr; }
    operator intptr_t() { return id; }
};

bool initRenderBackend(void *ptr);


// init
extern bool (*render_init)(void);
extern bool (*render_close)(void);
extern bool (*render_setMode)(int w, int h, bool full);

// state management
extern void (*render_pushState)(void);
extern void (*render_popState)(void);
extern void (*render_initState)(void);

// coloring
extern void (*render_color4f)(float r, float g, float b, float a);
extern void (*render_color4c)(char r, char g, char b, char a);
extern void (*render_colori)(int c);

// render target
extern void (*render_setTarget)(TextureHandle h);
extern void (*render_translate)(float x, float y);
extern void (*render_pixel)(float x, float y);
extern void (*render_line)(float x1, float y1, float x2, float y2);
extern void (*render_tex)(TextureHandle h, float x, float y);


// textures
extern int (*tex_width)(TextureHandle h);
extern int (*tex_height)(TextureHandle h);
extern TextureHandle (*tex_create)(void *data, size_t size);
extern TextureHandle (*tex_createRaw)(int w, int h, int components, void *pixels);
extern void (*tex_free)(TextureHandle h);


#endif
