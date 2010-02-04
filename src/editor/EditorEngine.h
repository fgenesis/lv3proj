#ifndef EDITORENGINE_H
#define EDITORENGINE_H

#include <set>

#include "Engine.h"
#include <guichan.hpp>
#include <guichan/sdl.hpp>



class EditorEngine : public Engine
{
public:
    EditorEngine();
    ~EditorEngine();

    virtual bool Setup(void);

    //virtual void OnMouseEvent(uint32 button, uint32 x, uint32 y, int32 rx, uint32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    //virtual void OnWindowEvent(bool active);
    virtual bool OnRawEvent(SDL_Event& evt);
    virtual void OnWindowResize(uint32 newx, uint32 newy);



protected:

    virtual void _Process(uint32 ms);
    virtual void _Render(void);

    std::set<gcn::Widget*> _widgets;

    gcn::Gui *_gcnGui;
    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLInput* _gcnInput;
    gcn::SDLImageLoader* _gcnImgLoader;

    gcn::Container *_topWidget;
    gcn::Font *_gcnFont;


};

#endif
