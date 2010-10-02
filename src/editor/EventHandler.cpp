#include "common.h"
#include "EditorEngine.h"
#include "TileLayer.h"


void EditorEngine::action(const gcn::ActionEvent& actionEvent)
{
    gcn::Widget *src = actionEvent.getSource();
    if(src == btnToggleTilebox)
    {
        ToggleTilebox();
    }
    else if(src == btnTiles)
    {
        ToggleTileWnd();
    }
    else if(src == btnToggleLayers)
    {
        ToggleLayerPanel();
    }
    else if(src == btnQuit)
    {
        _quit = true; // TODO: "Do you really want to...? YES DAMNIT!"
    }
    else if(src == btnSaveAs)
    {
        SaveCurrentMapAs("output.lvpm"); // TODO: show filename dialog
    }
    else if(src == btnLoad)
    {
        LoadMapFile("output.lvpm"); // TODO: show filename dialog
    }

}

void EditorEngine::mousePressed(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == panTilebox || src == wndTiles)
        {
            _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
            _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
            _selOverlayHighlight = true;
        }
    }

    if( (src == panTilebox && mouseEvent.getButton() == gcn::MouseEvent::RIGHT)
        || (src == panMain && mouseEvent.getButton() == gcn::MouseEvent::LEFT))
    {
        TileLayer *target = _GetActiveLayerForWidget(src);
        gcn::Rectangle rect = GetTargetableLayerTiles(mouseEvent.getX(), mouseEvent.getY(),
            _selLayerBorderRect.width, _selLayerBorderRect.height,
            _selLayer->GetArraySize(), _selLayer->GetArraySize());
        for(uint32 y = 0; y < uint32(rect.height); y++)
        {
            for(uint32 x = 0; x < uint32(rect.width); x++)
            {
                BasicTile *tile = _selLayer->GetTile(x,y);
                target->SetTile(x + rect.x, y + rect.y, tile);
            }
        }
    }
}

void EditorEngine::mouseReleased(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        _selOverlayHighlight = false;
        if(src == panTilebox || src == wndTiles)
        {
            _mouseStartX = mouseEvent.getX() - (mouseEvent.getX() % 16);
            _mouseStartY = mouseEvent.getY() - (mouseEvent.getY() % 16);
            UpdateSelection(mouseEvent.getSource());
            UpdateSelectionFrame(mouseEvent.getSource(), mouseEvent.getX(), mouseEvent.getY());
        }
    }
}

void EditorEngine::mouseExited(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _selOverlayShow = false;
    }
}

void EditorEngine::mouseClicked(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    // poll the buttons for a click
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        if(src == btnLayers[i])
        {
            if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
                SetActiveLayer(i);
            else if(mouseEvent.getButton() == gcn::MouseEvent::RIGHT)
                ToggleLayerVisible(i);

            return;
        }
    }

}

void EditorEngine::mouseMoved(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _mouseStartX = mouseEvent.getX();
        _mouseStartY = mouseEvent.getY();
        UpdateSelectionFrame(src, mouseEvent.getX(), mouseEvent.getY());
    }
    else
        _selOverlayShow = false;
}

void EditorEngine::mouseDragged(gcn::MouseEvent& mouseEvent)
{
    gcn::Widget *src = mouseEvent.getSource();
    if(mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == wndTiles || src == panTilebox || src == panMain)
        {
            UpdateSelectionFrame(src, mouseEvent.getX(), mouseEvent.getY());
        }
    }

    if(src == panMain && mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        TileLayer *target = _GetActiveLayerForWidget(src);
        gcn::Rectangle rect = GetTargetableLayerTiles(mouseEvent.getX(), mouseEvent.getY(),
            _selLayerBorderRect.width, _selLayerBorderRect.height,
            _selLayer->GetArraySize(), _selLayer->GetArraySize());
        for(uint32 y = 0; y < uint32(rect.height); y++)
        {
            for(uint32 x = 0; x < uint32(rect.width); x++)
            {
                BasicTile *tile = _selLayer->GetTile(x,y);
                target->SetTile(x + rect.x, y + rect.y, tile);
            }
        }
    }
}
