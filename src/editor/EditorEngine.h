#ifndef EDITORENGINE_H
#define EDITORENGINE_H

#include <set>

#include "Engine.h"
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include "GuichanExt.h"
#include "TileLayer.h"
#include "LayerMgr.h"


#define PREVIEWLAYER_MAX_SIZE 4



class EditorEngine : public Engine, public gcn::ActionListener, public gcn::MouseListener
{
protected:

    // inherited from gcn::ActionListener
    virtual void action(const gcn::ActionEvent& actionEvent);

    // inherited from gcn::MouseListener
    virtual void mousePressed(gcn::MouseEvent& mouseEvent);
    virtual void mouseDragged(gcn::MouseEvent& mouseEvent);
    virtual void mouseReleased(gcn::MouseEvent& mouseEvent);
    virtual void mouseMoved(gcn::MouseEvent& mouseEvent);
    virtual void mouseExited(gcn::MouseEvent& mouseEvent);


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

    void ClearWidgets(void);
    void SetupInterface(void);
    void SetupInterfaceLayers(void);
    void ToggleVisible(gcn::Widget *w);
    void ToggleTilebox(void);
    void ToggleTileWnd(void);
    void ToggleSelPreviewLayer(void);

    // note that rsrc height and width are expected to be bottom right x and y!
    gcn::Rectangle Get16pxAlignedFrame(gcn::Rectangle rsrc);
    void UpdateSelectionFrame(gcn::Widget *src, int x, int y);
    void UpdateSelection(gcn::Widget *src);


    gcn::Widget *RegWidget(gcn::Widget *w);
    gcn::Widget *AddWidgetTop(gcn::Widget *w);


protected:

    virtual void _Process(uint32 ms);
    virtual void _Render(void);

    void _DrawSelOverlay(void);

    std::set<gcn::Widget*> _widgets;

    gcn::Gui *_gcnGui;
    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLInput* _gcnInput;
    gcn::SDLImageLoader* _gcnImgLoader;

    gcn::Container *_topWidget;
    gcn::Font *_gcnFont;

    // mouse selection related
    gcn::Rectangle _selOverlayRect;
    gcn::Rectangle _selOverlayClip;
    bool _selOverlayShow;
    bool _selOverlayHighlight;
    int _mouseStartX;
    int _mouseStartY;

    // GUI elements - main gui
    gcn::Panel *panTilebox; // right panel
    gcn::Panel *panBottom; // bottom panel with all the buttons
    gcn::Button *btnQuit;
    gcn::Button *btnNew;
    gcn::Button *btnToggleTilebox;
    gcn::Button *btnSaveAs;
    gcn::Button *btnLoad;
    gcn::Button *btnData;
    gcn::Button *btnTiles;
    gcn::Window *wndTiles; // large tile window
    TileLayer *panTileboxLayer;

    // GUI elements - tile window
    gcn::Label *laTWCurFolder;
    gcn::Button *btnTWNext;
    gcn::Button *btnTWPrev;
    TileLayer *wndTilesLayer; // the layer where all the tiles for selection will be put

    // tiling and layers, main surface
    TileLayer *_selLayer;
    bool _selLayerShow;
    gcn::Rectangle _selLayerBorderRect;


};

#endif
