#ifndef EDITORENGINE_H
#define EDITORENGINE_H

#include <set>

#include "Engine.h"
#include <guichan.hpp>
#include <guichan/sdl.hpp>
#include "GuichanExt.h"
#include "TileLayer.h"
#include "LayerMgr.h"
#include "FileDialog.h"

class FileDialog;


#define PREVIEWLAYER_MAX_SIZE 4



class EditorEngine : public Engine, public gcn::ActionListener, public gcn::MouseListener, public FileDialogCallback
{
protected:

    // inherited from gcn::ActionListener
    virtual void action(const gcn::ActionEvent& ae);

    // inherited from gcn::MouseListener
    virtual void mousePressed(gcn::MouseEvent& me);
    virtual void mouseDragged(gcn::MouseEvent& me);
    virtual void mouseReleased(gcn::MouseEvent& me);
    virtual void mouseMoved(gcn::MouseEvent& me);
    virtual void mouseExited(gcn::MouseEvent& me);
    virtual void mouseClicked(gcn::MouseEvent& me);
    virtual void mouseWheelMovedDown(gcn::MouseEvent& me);
    virtual void mouseWheelMovedUp(gcn::MouseEvent& me);

    // helper
    void mouseWheelMoved(gcn::MouseEvent& me, bool up);

    // inherited from FileDialogCallback
    void FileChosenCallback(FileDialog *dlg);


public:
    EditorEngine();
    ~EditorEngine();

    virtual bool Setup(void);

    //virtual void OnMouseEvent(uint32 type, uint32 button, uint32 state, uint32 x, uint32 y, int32 rx, int32 ry);
    virtual void OnKeyDown(SDLKey key, SDLMod mod);
    virtual void OnKeyUp(SDLKey key, SDLMod mod);
    //virtual void OnWindowEvent(bool active);
    virtual bool OnRawEvent(SDL_Event& evt);
    virtual void OnWindowResize(uint32 newx, uint32 newy);

    void SaveData(void);
    void LoadData(void);

    void ClearWidgets(void);
    void SetupInterface(void);
    void SetupInterfaceLayers(void);
    void SetupEditorLayers(void);
    void ToggleVisible(gcn::Widget *w);
    void ToggleTilebox(void);
    void ToggleTileWnd(void);
    void ToggleSelPreviewLayer(void);
    void ToggleLayerPanel(void);
    void ToggleLayerVisible(uint32 layerId);
    void UpdateLayerButtonColors(void);
    void PanDrawingArea(int32 x, int32 y);
    void LoadPackages(void);
    void FillUseableTiles(void);

    void SetActiveLayer(uint32 layerId);

    void SetLeftMainDistance(uint32 dist);

    // note that rsrc height and width are expected to be bottom right x and y!
    gcn::Rectangle Get16pxAlignedFrame(gcn::Rectangle rsrc);
    void UpdateSelectionFrame(gcn::Widget *src, int x, int y);
    void UpdateSelection(gcn::Widget *src);
    gcn::Rectangle GetTargetableLayerTiles(uint32 baseX, uint32 baseY, uint32 addX, uint32 addY,
                                           uint32 maxwidth, uint32 maxheight, TileLayer *layer);

    gcn::Widget *RegWidget(gcn::Widget *w);
    gcn::Widget *AddWidgetTop(gcn::Widget *w);


protected:

    virtual void _Process(uint32 ms);
    virtual void _Render(void);

    void _DrawSelOverlay(void);

    TileLayer *_GetActiveLayerForWidget(gcn::Widget *src);

    std::set<gcn::Widget*> _widgets;

    gcn::Gui *_gcnGui;
    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLInput* _gcnInput;
    gcn::SDLImageLoader* _gcnImgLoader;

    gcn::Container *_topWidget;
    gcn::Font *_gcnFont;
    FileDialog *_fileDlg; // used file dialog window, hidden and shown as needed

    // mouse selection related
    gcn::Rectangle _selOverlayRect; // relative to _selOverlayClip
    gcn::Rectangle _selCurrentSelRect;
    gcn::Rectangle _selOverlayClip; // clipping area in which _selOverlayRect is drawn
    bool _selOverlayShow;
    bool _selOverlayHighlight;
    int _mouseLeftStartX; // related to left button
    int _mouseLeftStartY;
    int _mouseRightStartX; // related to right button
    int _mouseRightStartY;

    // GUI elements - main gui
    gcn::Panel *panMain;
    gcn::Panel *panTilebox; // right panel
    gcn::Panel *panBottom; // bottom panel with all the buttons
    gcn::Button *btnQuit;
    gcn::Button *btnNew;
    gcn::Button *btnToggleTilebox;
    gcn::Button *btnSaveAs;
    gcn::Button *btnLoad;
    gcn::Button *btnData;
    gcn::Button *btnTiles;
    gcn::Button *btnToggleLayers;
    gcn::Window *wndTiles; // large tile window
    TileLayer *panTileboxLayer;

    // GUI elements - tile window
    gcn::Label *laTWCurFolder;
    gcn::Button *btnTWNext;
    gcn::Button *btnTWPrev;
    TileLayer *wndTilesLayer; // the layer where all the tiles for selection will be put

    // tiling and layers, main surface, preview box
    TileLayer *_selLayer;
    bool _selLayerShow;
    gcn::Rectangle _selLayerBorderRect; // this defines where the selection preview box is drawn

    // tiling and layers, main surface, draw area
    uint32 _activeLayer;
    uint32 _visibleLayerMask;

    // layer settings
    gcn::Panel *panLayers;
    gcn::Button *btnLayers[LAYER_MAX];

    // gui misc config
    uint32 tileboxCols;

private:
    void SaveCurrentMapAs(const char *fn);
    bool LoadMapFile(const char *fn);


};

#endif
