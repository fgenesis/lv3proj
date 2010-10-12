#include "common.h"
#include "EditorEngine.h"
#include "TileLayer.h"
#include "FileDialog.h"


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
        _fileDlg->Open(true, "map");
    }
    else if(src == btnLoad)
    {
        _fileDlg->Open(false, "map");
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

    // TODO: clean up this mess, this must be possible to do easier!
    if(src == panMain && me.getButton() == gcn::MouseEvent::LEFT)
    {
        HandlePaintOnWidget(src, me.getX(), me.getY(), true);
    }
    else if(src == panTilebox && me.getButton() == gcn::MouseEvent::RIGHT)
    {
        HandlePaintOnWidget(src, me.getX(), me.getY(), false);
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
            HandlePaintOnWidget(src, me.getX(), me.getY(), true);
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

void EditorEngine::FileChosenCallback(FileDialog *dlg)
{
    DEBUG(ASSERT(dlg == _fileDlg)); // just to be sure it works, there is only one FileDialog object anyways

    logdebug("FileChosenCallback, operation '%s'", dlg->GetOperation());

    // Load/Save map file
    if(!strcmp(dlg->GetOperation(), "map"))
    {
        std::string& fn = dlg->GetFileName();
        if(dlg->IsSave())
        {
            logdetail("Saving map to '%s'", fn.c_str());
            SaveCurrentMapAs(fn.c_str());
        }
        else
        {
            logdetail("Loading map from '%s'", fn.c_str());
            if(!LoadMapFile(fn.c_str()))
            {
                logerror("'%s' can't be loaded, invalid?", fn.c_str());
                return; // keep file dialog open
            }
        }
    }

    dlg->Close();

}
