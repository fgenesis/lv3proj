#ifndef GCN_EXT_GREEDYTEXTFIELD
#define GCN_EXT_GREEDYTEXTFIELD

#include "guichan.hpp"

namespace gcn
{

class GreedyTextField : public TextField
{
public:
    virtual void keyPressed(KeyEvent& keyEvent);
};

}

#endif
