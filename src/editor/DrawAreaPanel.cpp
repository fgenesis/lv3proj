#include "common.h"
#include "EditorEngine.h"
#include "DrawAreaPanel.h"
#include "LayerPanel.h"

DrawAreaPanel::DrawAreaPanel(EditorEngine *engine)
: TileLayerPanel(engine), _activeLayer(0)
{
    _preview = new TileLayerPanel(engine);
    _preview->removeMouseListener(_preview); // it should not react to anything
    _preview->setForegroundColor(gcn::Color(255, 0, 255));
    _preview->setBackgroundColor(gcn::Color(0,0,0,0)); // full transparent
    add(_preview);
    _preview->setPosition(0, 0);
    TileLayer *prev = new TileLayer;
    prev->target = engine->GetSurface();
    prev->visible = true;
    _preview->GetTiles().push_back(prev);
}

DrawAreaPanel::~DrawAreaPanel()
{
    delete _preview;
}

TileLayer *DrawAreaPanel::GetPaintableTiles(void)
{
    DEBUG(ASSERT(_preview->GetTiles().size() == 1));
    return _preview->GetTiles()[0];
}

void DrawAreaPanel::logic(void)
{
    if(IsCovered())
        return;

    TileLayerPanel::logic();

    Camera cam = _engine->GetCamera();
    _blockOffsX = -cam.x; // if camera goes top-left (negative), the screen pans to bottom-right (positive)...
    _blockOffsY = -cam.y;
    int maxwidth = _blockW * 4;
    int maxheight = _blockH * 4;
    _preview->setWidth(std::min(_frame.width, maxwidth));
    _preview->setHeight(std::min(_frame.height, maxheight));

    uint32 previewOffsX = _engine->GetLayerPanel()->isVisible() ? _engine->GetLayerPanel()->getWidth() : 0;
    _preview->setX(previewOffsX);
    //GetPaintableTiles()->xoffs = previewOffsX;

    logicChildren();
}

void DrawAreaPanel::draw(gcn::Graphics *g)
{
    if(IsCovered())
        return;

    // always show currently selected layer
    TileLayer *activeLayer = _mgr->GetLayer(GetActiveLayerId());
    bool curVis = activeLayer->visible;
    activeLayer->visible = true;

    TileLayerPanel::draw(g);

    activeLayer->visible = curVis;

    
    // draw box around the drawing area
    uint32 pixdim = _mgr->GetMaxPixelDim() + 2;
    Camera cam = _engine->GetCamera();
    gcn::Rectangle clip(
        -cam.x - 1,
        -cam.y - 1,
        pixdim,
        pixdim);


    g->setColor(gcn::Color(255, 0, 0, 180));
    g->pushClipArea(gcn::Rectangle(0,0,_engine->GetResX(), _engine->GetResY()));
    g->drawRectangle(clip);
    // draw 2nd rect around it (makes it thicker)
    clip.x--;
    clip.y--;
    clip.width += 2;
    clip.height+= 2;
    g->drawRectangle(clip);
    g->popClipArea();
    

    drawChildren(g);
}

void DrawAreaPanel::mouseReleased(gcn::MouseEvent& me)
{
    _initRect(me.getX(), me.getY(), false);
    _hilightSelRect = false;
}

void DrawAreaPanel::mouseMoved(gcn::MouseEvent& me)
{
    TileLayerPanel::mouseMoved(me);

    // limit drawing area; do not show selection rect outside
    // top, left
    if(_frame.x < _blockOffsX || _frame.y < _blockOffsY)
        _showSelRect = false;
    // bottom, right
    if(_frame.x - _blockOffsX + _blockW > (int32)_mgr->GetMaxPixelDim() || _frame.y - _blockOffsY + _blockH > (int32)_mgr->GetMaxPixelDim())
        _showSelRect = false;
}

void DrawAreaPanel::SetActiveLayer(uint32 i)
{
    _activeLayer = i;
    _engine->GetLayerPanel()->UpdateSelection();
}

// called when tiles are to be drawn on a layer
void DrawAreaPanel::_DrawSelTiles(void)
{
    // only allow drawing while the selection rect is visible
    if(!_showSelRect)
        return;

    int32 w = GetSelBlocksW();
    int32 h = GetSelBlocksH();
    int32 x = GetSelBlocksX();
    int32 y = GetSelBlocksY();

    // offset correction - if the camera is not at (0, 0).
    // TODO: make this less hacky
    Camera cam = _engine->GetCamera();
    if(cam.x >= 0)
        x += ((cam.x + (int32)GetBlockW() - 1) / (int32)GetBlockW());
    else
        x += (cam.x / (int32)GetBlockW());

    if(cam.y >= 0)
        y += ((cam.y + (int32)GetBlockH() - 1) / (int32)GetBlockH());
    else
        y += (cam.y / (int32)GetBlockH());

    TileLayer *target = _mgr->GetLayer(_activeLayer);
    GetPaintableTiles()->CopyTo(0, 0, target, x, y, w, h);

    // update used tile counter in layer panel
    _engine->GetLayerPanel()->UpdateStats(target);

}

void DrawAreaPanel::mousePressed( gcn::MouseEvent& me)
{
    if(me.getButton() == gcn::MouseEvent::LEFT)
        _DrawSelTiles();

    TileLayerPanel::mousePressed(me);
}

void DrawAreaPanel::mouseDragged( gcn::MouseEvent& me)
{
    if(me.getButton() == gcn::MouseEvent::LEFT)
        _DrawSelTiles();

    TileLayerPanel::mouseDragged(me);
}
