#include "common.h"
#include "LayerPanel.h"
#include "LayerMgr.h"
#include "EditorEngine.h"
#include "GuichanExt.h"
#include "DrawAreaPanel.h"


LayerPanel::LayerPanel(EditorEngine *engine, uint32 width, uint32 height)
: gcn::Panel(4,4), _engine(engine) // 4 pixels h and v spacing
{
    setBackgroundColor(gcn::Color(0,75,0,255));
    setForegroundColor(gcn::Color(0,200,0,255));
    setSize(width, height);
    setVisible(false);
    SetMaxSlots(2, -1);
    gcn::Button *btn;

    uint32 w = getWidth() / 2 - (GetSpacingX() * 2);
    uint32 h = uint32(getFont()->getHeight() * 1.5f);

    // add buttons in such a way that the left column contains buttons for layers 0-15,
    // and the right column those for layers 16-31.
    for(uint32 i = 0; i < LAYER_MAX / 2; i++)
    {
        btn = new gcn::Button();
        btn->setSize(w, h);
        btn->addMouseListener(this);
        add(btn);
        btnLayers[i] = btn;

        btn = new gcn::Button();
        btn->setSize(w, h);
        btn->addMouseListener(this);
        add(btn);
        btnLayers[i + (LAYER_MAX / 2)] = btn;
    }
    gcn::Font *font = engine->GetLargeFont();

    lDesc = new gcn::Label("Desc");
    lDesc->setFont(font);
    lDesc->setSize(getWidth(), h);
    add(lDesc);

    lInfo = new gcn::Label("Info");
    lInfo->setFont(font);
    lInfo->setSize(getWidth(), h);
    add(lInfo);

    tfName = new gcn::GreedyTextField();
    tfName->setSize(getWidth() - 10, h);
    tfName->addActionListener(this);
    add(tfName);

    cbVisible = new gcn::CheckBox("visible");
    cbVisible->setSize(getWidth(), h);
    cbVisible->setFont(font);
    cbVisible->addActionListener(this);
    add(cbVisible);

    // .. room for more widgets ...


    // these are added last
    btnAllVisible = new gcn::Button("All visible");
    btnAllVisible->setSize(w, h);
    btnAllVisible->addActionListener(this);
    add(btnAllVisible);
    btnAllVisible->setPosition(getX() + GetSpacingX(), getHeight() - h - (GetSpacingY() * 2));

    btnNoneVisible = new gcn::Button("None visible");
    btnNoneVisible->setSize(w, h);
    btnNoneVisible->addActionListener(this);
    add(btnNoneVisible);
    btnNoneVisible->setPosition(getX() + (GetSpacingX() * 2) + w, getHeight() - h - (GetSpacingY() * 2));

}

LayerPanel::~LayerPanel()
{
    for(uint32 i = 0; i < LAYER_MAX; i++)
        delete btnLayers[i];
    delete lDesc;
    delete lInfo;
    delete cbVisible;
    delete tfName;
    delete btnAllVisible;
    delete btnNoneVisible;
}

void LayerPanel::mouseClicked(gcn::MouseEvent& me)
{
    gcn::Widget *src = me.getSource();
    // poll the buttons for a click
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        if(src == btnLayers[i])
        {
            if(me.getButton() == gcn::MouseEvent::LEFT)
                _engine->GetDrawPanel()->SetActiveLayer(i);
            else if(me.getButton() == gcn::MouseEvent::RIGHT)
                _engine->ToggleLayerVisible(i);

            return;
        }
    }
}

void LayerPanel::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();

    if(src == cbVisible)
    {
        _engine->GetLayerMgr()->GetLayer(_engine->GetDrawPanel()->GetActiveLayerId())->visible = cbVisible->isSelected();
        UpdateSelection();
    }
    else if(src == tfName)
    {
        _engine->GetLayerMgr()->GetLayer(_engine->GetDrawPanel()->GetActiveLayerId())->name = tfName->getText();
        focusNext(); // move focus away, else almost impossible to escape from textfield widget
        UpdateSelection();
    }
    else if(src == btnAllVisible)
    {
        for(uint32 i = 0; i < LAYER_MAX; i++)
            _engine->GetLayerMgr()->GetLayer(i)->visible = true;
        UpdateSelection();
    }
    else if(src == btnNoneVisible)
    {
        for(uint32 i = 0; i < LAYER_MAX; i++)
            _engine->GetLayerMgr()->GetLayer(i)->visible = false;
        UpdateSelection();
    }

}

void LayerPanel::UpdateSelection(void)
{
    TileLayer *activeLayer = NULL;
    uint32 a = _engine->GetDrawPanel()->GetActiveLayerId();
    for(uint32 i = 0; i < LAYER_MAX; i++)
    {
        TileLayer *layer = _engine->GetLayerMgr()->GetLayer(i);
        uint8 alpha = layer->visible ? 255 : 150;
        if(i == a)
        {
            activeLayer = layer;
            btnLayers[i]->setBaseColor(gcn::Color(180,255,180,alpha));
        }
        else
        {
            btnLayers[i]->setBaseColor(gcn::Color(128,128,144,alpha));
        }
        btnLayers[i]->setCaption(layer->name);
    }
    ASSERT(activeLayer);

    char buf[30];
    sprintf(buf, a < 10 ? "Layer 0%u:" : "Layer %u:", a);
    lDesc->setCaption(buf);

    tfName->setText(activeLayer->name);
    cbVisible->setSelected(activeLayer->visible);

    UpdateStats(activeLayer);
}

void LayerPanel::UpdateStats(TileLayer *layer)
{
    char buf[30];
    sprintf(buf, "Tiles used: %u", layer->UsedTiles());
    lInfo->setCaption(buf);
}
