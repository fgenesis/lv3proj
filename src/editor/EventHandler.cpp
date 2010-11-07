#include "common.h"
#include "EditorEngine.h"
#include "TileLayer.h"
#include "FileDialog.h"
#include "TileboxPanel.h"
#include "DrawAreaPanel.h"


void EditorEngine::action(const gcn::ActionEvent& ae)
{
}

void EditorEngine::keyPressed(gcn::KeyEvent& ke)
{
    switch(ke.getKey().getValue())
    {
    case 't': case 'T':
        ToggleTileWnd();
        break;

    case 'g': case 'G':
        ToggleTilebox();
        break;

    case 'p': case 'P':
        ToggleSelPreviewLayer();
        break;

    case 'l': case 'L':
        ToggleLayerPanel();
        break;

    case 'm': case 'M':
        panMain->SetActiveLayer((panMain->GetActiveLayerId() + 1) % LAYER_MAX);
        break;

    case 'n': case 'N':
        panMain->SetActiveLayer((panMain->GetActiveLayerId() + LAYER_MAX - 1) % LAYER_MAX);
        break;

        // TODO: below, fix for tile size != 16
    case gcn::Key::UP:
        PanDrawingArea(0, -16 * (ke.isControlPressed() ? 5 : 1) );
        break;

    case gcn::Key::DOWN:
        PanDrawingArea(0, 16 * (ke.isControlPressed() ? 5 : 1) );
        break;

    case gcn::Key::LEFT:
        PanDrawingArea(-16 * (ke.isControlPressed() ? 5 : 1) , 0);
        break;

    case gcn::Key::RIGHT:
        PanDrawingArea(16 * (ke.isControlPressed() ? 5 : 1) , 0);
        break;
    }
}

void EditorEngine::mousePressed(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();

    if(me.getButton() == gcn::MouseEvent::RIGHT)
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
        //HandlePaintOnWidget(src, me.getX(), me.getY(), true);
    }
    else if(src == panTilebox && me.getButton() == gcn::MouseEvent::RIGHT)
    {
        //HandlePaintOnWidget(src, me.getX(), me.getY(), false);
    }
}

void EditorEngine::mouseDragged(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();

    if(src == panMain)
    {
        if(me.getButton() == gcn::MouseEvent::LEFT) // paint on left-click
        {
            //HandlePaintOnWidget(src, me.getX(), me.getY(), true);
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
