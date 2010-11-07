#include "guichan.hpp"
#include "GuichanExt.h"
#include "LayerMgr.h"

class EditorEngine;

class LayerPanel : public gcn::Panel, public gcn::MouseListener, public gcn::ActionListener
{
public:
    LayerPanel(EditorEngine *engine, uint32 width, uint32 height);
    virtual ~LayerPanel();

    virtual void mouseClicked(gcn::MouseEvent& me); // from gcn::MouseListener
    virtual void action(const gcn::ActionEvent& ae); // from gcn::ActionListener
    void UpdateSelection(void);
    void UpdateStats(TileLayer *layer);

protected:
    gcn::Button *btnLayers[LAYER_MAX];
    gcn::Label *lDesc; // layer ID
    gcn::Label *lInfo; // contains infos like tile count
    gcn::TextField *tfName; // to edit the name
    gcn::CheckBox *cbVisible;
    gcn::Button *btnAllVisible;
    gcn::Button *btnNoneVisible;
    
    EditorEngine *_engine;
};
