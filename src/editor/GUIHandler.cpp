#include "common.h"
#include "EditorEngine.h"
#include "ResourceMgr.h"
#include "GuichanExt.h"
#include "SDLImageLoaderManaged.h"
#include "FileDialog.h"
#include "LayerPanel.h"
#include "TileboxPanel.h"
#include "DrawAreaPanel.h"
#include "BottomBarPanel.h"
#include "TileWindow.h"


void EditorEngine::ClearWidgets(void)
{
    _topWidget->clear();
    for(std::set<gcn::Widget*>::iterator it = _widgets.begin(); it != _widgets.end(); it++)
    {
        delete *it;
    }
    _widgets.clear();

}

// registers a widget for garbage collection
gcn::Widget *EditorEngine::RegWidget(gcn::Widget *w)
{
    _widgets.insert(w);
    return w;
}

// adds a widget to the top widget and registers it for garbage collection
gcn::Widget *EditorEngine::AddWidgetTop(gcn::Widget *w)
{
    _topWidget->add(w);
    return RegWidget(w);
}

// place widgets based on resolution.
// to be called on each resize event
void EditorEngine::SetupInterface(void)
{
    ClearWidgets();
    _topWidget->setDimension(gcn::Rectangle(0, 0, GetResX(), GetResY()));
    _topWidget->setOpaque(false);
    _topWidget->setBaseColor(gcn::Color(0,0,0,255));


    // -- bottom panel start --
    panBottom = new BottomBarPanel(this);
    panBottom->setPosition(0, GetResY() - panBottom->getHeight());
    AddWidgetTop(panBottom);
    uint32 freeHeight = GetResY() - panBottom->getHeight();
    // -- bottom panel end --


    // -- right panel start --
    panTilebox = new TileboxPanel(this);
    panTilebox->SetBlockSize(16,16);
    panTilebox->setForegroundColor(gcn::Color(200,200,200,255));
    panTilebox->setBackgroundColor(gcn::Color(30,30,30,200));
    panTilebox->setSize(tileboxCols * panTilebox->GetBlockW(), freeHeight);
    panTilebox->SetMaxSlots(tileboxCols, -1);
    panTilebox->addMouseListener(this);

    AddWidgetTop(panTilebox)->setPosition(GetResX() - panTilebox->getWidth(), 0);
    // -- right panel end --

    // -- left layer panel start --
    panLayers = new LayerPanel(this, 180, GetResY() - panBottom->getHeight());
    panLayers->setPosition(0, 0);
    AddWidgetTop(panLayers);
    // -- left layer panel end --

    // -- main panel start --
    panMain = new DrawAreaPanel(this);
    panMain->SetLayerMgr(_layermgr);
    panMain->SetBlockSize(16, 16); // TODO: change this
    panMain->SetDraggable(false);
    panMain->setForegroundColor(gcn::Color(255,255,255,255));
    panMain->setBackgroundColor(gcn::Color(0,0,0,255));
    panMain->setSize(GetResX(), GetResY() - panBottom->getHeight());
    panMain->SetMaxSlots(-1,-1);
    panMain->addMouseListener(this);

    AddWidgetTop(panMain)->setPosition(0, 0);
    // -- main panel end --


    // -- tile window start --
    wndTiles = new TileWindow(this);
    wndTiles->setSize(GetResX(), GetResY() - panBottom->getHeight());
    AddWidgetTop(wndTiles);
    // -- tile window end --

    // -- file dialog window start --
    if(!_fileDlg)
    {
        _fileDlg = new FileDialog();
        _fileDlg->SetCallback(this);
    }
    _topWidget->add(_fileDlg);
    // -- file dialog window end --

    panMain->requestMoveToBottom();

    // speed enhancement - some widgets are covered if the big tile window is visible
    panMain->SetCoveredBy(wndTiles);
    panTilebox->SetCoveredBy(wndTiles);
    panLayers->SetCoveredBy(wndTiles);


    SetupInterfaceLayers();
}

void EditorEngine::SetupInterfaceLayers(void)
{
    uint32 resmax = std::max(GetResX(), GetResY());
    uint32 tilesmax = resmax / 16; // TODO: fix for tile size != 16
    TileLayer *tl;

    {
        std::vector<TileLayer*>& tblayerv = panTilebox->GetTiles();
        if(tblayerv.empty())
        {
            tl = new TileLayer();
            tl->target = GetSurface();
            tl->visible = true;

            tblayerv.push_back(tl);
        }
        else
        {
            tl = tblayerv[0];
        }

        tl->Resize(tilesmax);
    }

    {
        std::vector<TileLayer*>& tblayerv = wndTiles->GetTilesPanel()->GetTiles();
        if(tblayerv.empty())
        {
            tl = new TileLayer();
            tl->target = GetSurface();
            tl->visible = true;

            tblayerv.push_back(tl);
        }
        else
        {
            tl = tblayerv[0];
        }

        tl->Resize(tilesmax);
    }

    
}

void EditorEngine::ToggleVisible(gcn::Widget *w)
{
    w->setVisible(!w->isVisible());
}

void EditorEngine::ToggleSelPreviewLayer(void)
{
    TileLayerPanel *preview = panMain->GetPreview();
    preview->setVisible(!preview->isVisible());
}

void EditorEngine::ToggleTilebox(void)
{
    ToggleVisible(panTilebox);
}

void EditorEngine::ToggleTileWnd(void)
{
    ToggleVisible(wndTiles);
}

void EditorEngine::ToggleLayerPanel(void)
{
    ToggleVisible(panLayers);
}

void EditorEngine::ToggleLayerVisible(uint32 layerId)
{
    TileLayer *layer = _layermgr->GetLayer(layerId);
    layer->visible = !layer->visible;
    panLayers->UpdateSelection();
}
