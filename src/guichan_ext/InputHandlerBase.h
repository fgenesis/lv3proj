#ifndef GCN_EXT_INPUTHANDLERBASE_H
#define GCN_EXT_INPUTHANDLERBASE_H

#include "guichan/mouselistener.hpp"
#include "guichan/keylistener.hpp"

namespace gcn {

class InputHandlerBase : public MouseListener, public KeyListener
{
public:
    InputHandlerBase(InputHandlerBase *subH = NULL): subHandler(subH), forward(true) {}
    virtual ~InputHandlerBase() {}

    virtual void mousePressed(MouseEvent& e)  { if(subHandler && forward) subHandler->mousePressed(e); }
    virtual void mouseDragged(MouseEvent& e)  { if(subHandler && forward) subHandler->mouseDragged(e); }
    virtual void mouseMoved(MouseEvent& e)    { if(subHandler && forward) subHandler->mouseMoved(e); }
    virtual void mouseReleased(MouseEvent& e) { if(subHandler && forward) subHandler->mouseReleased(e); }
    virtual void mouseExited(MouseEvent& e)   { if(subHandler && forward) subHandler->mouseExited(e); }
    virtual void mouseEntered(MouseEvent& e)  { if(subHandler && forward) subHandler->mouseEntered(e); }
    virtual void mouseWheelMovedUp(MouseEvent& e)   { mouseWheel(e, true); }
    virtual void mouseWheelMovedDown(MouseEvent& e) { mouseWheel(e, false); }

    // for convenience
    virtual void mouseWheel(MouseEvent& e, bool up) { if(subHandler && forward) subHandler->mouseWheel(e, up); }

    virtual void keyPressed(KeyEvent& e)      { if(subHandler && forward) subHandler->keyPressed(e); }
    virtual void keyReleased(KeyEvent& e)     { if(subHandler && forward) subHandler->keyReleased(e); }
    
    InputHandlerBase *subHandler;
    bool forward;
};

} // end namespace gcn

#endif
