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

// to be called only once!
void EditorEngine::_CreateInterfaceWidgets(void)
{
    _topWidget->setOpaque(false);
    _topWidget->setBaseColor(gcn::Color(0,0,0,255));

    panBottom = new BottomBarPanel(this);
    panBottom->setPosition(0, GetResY() - panBottom->getHeight());
    _topWidget->add(panBottom);

    panTilebox = new TileboxPanel(this);
    panTilebox->setForegroundColor(gcn::Color(200,200,200,255));
    panTilebox->setBackgroundColor(gcn::Color(30,30,30,200));
    panTilebox->addMouseListener(this);
    _topWidget->add(panTilebox);

    panLayers = new LayerPanel(this, 180, GetResY() - panBottom->getHeight()); // use the same sizes below
    panLayers->setPosition(0, 0);
    panLayers->setVisible(false);
    _topWidget->add(panLayers);

    panMain = new DrawAreaPanel(this);
    panMain->SetLayerMgr(_layermgr);
    panMain->SetDraggable(false);
    panMain->setForegroundColor(gcn::Color(255,255,255,255));
    panMain->setBackgroundColor(gcn::Color(0,0,0,255));
    panMain->SetMaxSlots(-1,-1);
    panMain->addMouseListener(this);
    panMain->setPosition(0, 0);
    _topWidget->add(panMain);

    wndTiles = new TileWindow(this);
    wndTiles->setVisible(false);
    _topWidget->add(wndTiles);

    _fileDlg = new FileDialog();
    _fileDlg->SetCallback(this);
    _topWidget->add(_fileDlg);


    panMain->requestMoveToBottom();

    // speed enhancement - some widgets are covered if the big tile window is visible
    panMain->SetCoveredBy(wndTiles);
    panTilebox->SetCoveredBy(wndTiles);
    panLayers->SetCoveredBy(wndTiles);
}

// place widgets based on resolution.
// to be called on each resize event
// widgets must NOT be (re-)created here!
void EditorEngine::SetupInterface(void)
{
    _topWidget->setDimension(gcn::Rectangle(0, 0, GetResX(), GetResY()));

    panBottom->setPosition(0, GetResY() - panBottom->getHeight());
    panBottom->setWidth(GetResX());
    uint32 freeHeight = GetResY() - panBottom->getHeight();

    panTilebox->SetBlockSize(16,16);
    panTilebox->setSize(tileboxCols * panTilebox->GetBlockW(), freeHeight);
    panTilebox->SetMaxSlots(tileboxCols, -1);
    panTilebox->setPosition(GetResX() - panTilebox->getWidth(), 0);

    panLayers->setSize(180, GetResY() - panBottom->getHeight()); // use the same sizes above

    panMain->SetBlockSize(16, 16); // TODO: change this
    panMain->setSize(GetResX(), GetResY());

    wndTiles->setSize(GetResX(), GetResY() - panBottom->getHeight());
    wndTiles->GetTilesPanel()->SetBlockSize(panMain->GetBlockW(), panMain->GetBlockH());

    SetupInterfaceLayers();
}

// create TileLayers for tilebox and tile window if not present,
// and resize them if required
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

        if(tl->GetArraySize() < tilesmax)
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
        if(tl->GetArraySize() < tilesmax)
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
