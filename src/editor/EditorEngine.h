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
class LayerPanel;
class TileLayerPanel;
class TileboxPanel;
class DrawAreaPanel;
class BottomBarPanel;
class TileWindow;


#define PREVIEWLAYER_MAX_SIZE 4



class EditorEngine : public Engine, public gcn::ActionListener, public gcn::MouseListener,
    public gcn::KeyListener, public FileDialogCallback
{
protected:

    // inherited from gcn::ActionListener
    virtual void action(const gcn::ActionEvent& ae);

    // inherited from gcn::KeyListener
    virtual void keyPressed(gcn::KeyEvent& ke);

    // inherited from gcn::MouseListener
    virtual void mousePressed(gcn::MouseEvent& me);
    virtual void mouseDragged(gcn::MouseEvent& me);
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

    inline LayerMgr *GetLayerMgr(void) { return _layermgr; }
    inline FileDialog *GetFileDlg(void) { return _fileDlg; }

    void ChangeLayerMgr(LayerMgr *mgr);

    // note that rsrc height and width are expected to be bottom right x and y!
    void UpdateSelection(gcn::Widget *src);
    gcn::Rectangle GetTargetableLayerTiles(uint32 baseX, uint32 baseY, uint32 addX, uint32 addY,
                                           uint32 maxwidth, uint32 maxheight, TileLayer *layer);

    gcn::Widget *RegWidget(gcn::Widget *w);
    gcn::Widget *AddWidgetTop(gcn::Widget *w);

    inline gcn::Font *GetLargeFont(void) { return _largeFont; }

    inline LayerPanel *GetLayerPanel(void) { return panLayers; }
    inline DrawAreaPanel *GetDrawPanel(void) { return panMain; }
    inline TileboxPanel *GetTileboxPanel(void) { return panTilebox; }


protected:

    virtual void _Process(uint32 ms);
    virtual void _Render(void);

    gcn::Font *_LoadFont(const char *infofile, const char *gfxfile);

    std::set<gcn::Widget*> _widgets;

    gcn::Gui *_gcnGui;
    gcn::SDLGraphics* _gcnGfx;
    gcn::SDLInput* _gcnInput;
    gcn::SDLImageLoader* _gcnImgLoader;
    gcn::Font *_largeFont;

    gcn::Container *_topWidget;
    gcn::Font *_gcnFont;
    FileDialog *_fileDlg; // used file dialog window, hidden and shown as needed

    // mouse panning related (right button)
    int _mouseRightStartX;
    int _mouseRightStartY;

    // GUI elements - main gui
    DrawAreaPanel *panMain;
    TileboxPanel *panTilebox; // right panel
    BottomBarPanel *panBottom; // bottom panel with all the buttons
    TileWindow *wndTiles; // large tile window
    

    // GUI elements - tile window
    gcn::Label *laTWCurFolder;
    gcn::Button *btnTWNext;
    gcn::Button *btnTWPrev;
    TileLayer *wndTilesLayer; // the layer where all the tiles for selection will be put

    // layer settings
    LayerPanel *panLayers;

    // gui misc config
    uint32 tileboxCols;

private:
    void SaveCurrentMapAs(const char *fn);
    bool LoadMapFile(const char *fn);


};

#endif
