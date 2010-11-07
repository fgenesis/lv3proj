#ifndef TILEWINDOW_H
#define TILEWINDOW_H

#include "guichan/widgets/window.hpp"
#include "guichan/actionlistener.hpp"

class EditorEngine;

class TileWindow : public gcn::Window, public gcn::ActionListener
{
public:
    TileWindow(EditorEngine *engine);
    virtual ~TileWindow();

    virtual void action(const gcn::ActionEvent& ae);

protected:
    EditorEngine *_engine;

};

#endif
