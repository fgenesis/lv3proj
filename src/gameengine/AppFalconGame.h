#ifndef APPFALCONGAME_H
#define APPFALCONGAME_H

#include "AppFalcon.h"

class AppFalconGame : public AppFalcon
{
public:
    AppFalconGame(Engine *e);


protected:
    virtual void _LoadModules(void);

};

#endif
