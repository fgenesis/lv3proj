#include "common.h"
#include "EditorEngine.h"
#include "TileLayer.h"


void EditorEngine::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();
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

void EditorEngine::mousePressed(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    if(me.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == panTilebox || src == wndTiles)
        {
            _mouseLeftStartX = me.getX() - (me.getX() % 16);
            _mouseLeftStartY = me.getY() - (me.getY() % 16);
            _selOverlayHighlight = true;
        }
    }
    else if(me.getButton() == gcn::MouseEvent::RIGHT)
    {
        if(src == panMain)
        {
            _mouseRightStartX = me.getX();
            _mouseRightStartY = me.getY();
        }
    }

    if( (src == panTilebox && me.getButton() == gcn::MouseEvent::RIGHT)
        || (src == panMain && me.getButton() == gcn::MouseEvent::LEFT))
    {
        TileLayer *target = _GetActiveLayerForWidget(src);
        gcn::Rectangle rect = GetTargetableLayerTiles(me.getX() + src->getX(), me.getY() + src->getY(),
            _selLayerBorderRect.width, _selLayerBorderRect.height,
            _selLayer->GetArraySize(), _selLayer->GetArraySize(), target);
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

void EditorEngine::mouseReleased(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    if(me.getButton() == gcn::MouseEvent::LEFT)
    {
        _selOverlayHighlight = false;
        if(src == panTilebox || src == wndTiles)
        {
            _mouseLeftStartX = me.getX() - (me.getX() % 16);
            _mouseLeftStartY = me.getY() - (me.getY() % 16);
            UpdateSelection(me.getSource());
            UpdateSelectionFrame(me.getSource(), me.getX(), me.getY());
        }
    }
}

void EditorEngine::mouseExited(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _selOverlayShow = false;
    }
}

void EditorEngine::mouseClicked(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    // poll the buttons for a click
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        if(src == btnLayers[i])
        {
            if(me.getButton() == gcn::MouseEvent::LEFT)
                SetActiveLayer(i);
            else if(me.getButton() == gcn::MouseEvent::RIGHT)
                ToggleLayerVisible(i);

            return;
        }
    }

}

void EditorEngine::mouseMoved(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    if(src == wndTiles || src == panTilebox || src == panMain)
    {
        _mouseLeftStartX = me.getX();
        _mouseLeftStartY = me.getY();
        UpdateSelectionFrame(src, me.getX(), me.getY());
    }
    else
        _selOverlayShow = false;
}

void EditorEngine::mouseDragged(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    //if(me.getButton() == gcn::MouseEvent::LEFT)
    {
        if(src == wndTiles || src == panTilebox || src == panMain)
        {
            UpdateSelectionFrame(src, me.getX(), me.getY());
        }
    }

    if(src == panMain)
    {
        if(me.getButton() == gcn::MouseEvent::LEFT) // paint on left-click
        {
            TileLayer *target = _GetActiveLayerForWidget(src);
            gcn::Rectangle rect = GetTargetableLayerTiles(me.getX() + src->getX(), me.getY() + src->getY(),
                _selLayerBorderRect.width, _selLayerBorderRect.height,
                _selLayer->GetArraySize(), _selLayer->GetArraySize(), target);
            for(uint32 y = 0; y < uint32(rect.height); y++)
            {
                for(uint32 x = 0; x < uint32(rect.width); x++)
                {
                    BasicTile *tile = _selLayer->GetTile(x,y);
                    target->SetTile(x + rect.x, y + rect.y, tile);
                }
            }
        }
        else if(me.getButton() == gcn::MouseEvent::RIGHT) // pan on right-click
        {
            int32 xdiff = _mouseRightStartX - me.getX();
            int32 ydiff = _mouseRightStartY - me.getY();
            _mouseRightStartX = me.getX();
            _mouseRightStartY = me.getY();
            uint32 multi = me.isControlPressed() ? 5 : 1;
            PanDrawingArea(xdiff * multi, ydiff * multi);
        }
    }
}

void EditorEngine::mouseWheelMovedDown(gcn::MouseEvent& me)
{
    mouseWheelMoved(me, false);
}

void EditorEngine::mouseWheelMovedUp(gcn::MouseEvent& me)
{
    mouseWheelMoved(me, true);
}

void EditorEngine::mouseWheelMoved(gcn::MouseEvent& me, bool up)
{
    gcn::Widget *src = me.getSource();
    if(src == panMain)
    {
        int32 pan = (me.isControlPressed() ? 5*3 : 3) * (up ? -16 : 16);
        if(me.isShiftPressed())
            PanDrawingArea(pan, 0);
        else
            PanDrawingArea(0, pan);
    }
}

