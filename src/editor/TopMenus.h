#ifndef TOPMENUS_H
#define TOPMENUS_H

#include "GuichanExt.h"

class MenuEngineAdapter : public gcn::PopupMenu
{
protected:
    MenuEngineAdapter(EditorEngine *e) : gcn::PopupMenu(), _engine(e) {}
    EditorEngine *_engine;
};

class TopFileMenu : public MenuEngineAdapter
{
public:
    TopFileMenu(EditorEngine *e);
    virtual ~TopFileMenu();
    virtual void usedEntry(unsigned int which);
};

class TopEditMenu : public MenuEngineAdapter
{
public:
    TopEditMenu(EditorEngine *e);
    virtual ~TopEditMenu();
    virtual void usedEntry(unsigned int which);
};


class TopToolsMenu : public MenuEngineAdapter
{
public:
    TopToolsMenu(EditorEngine *e);
    virtual ~TopToolsMenu();
    virtual void usedEntry(unsigned int which);

    void toggleTilebox(bool on);
    void toggleLayers(bool on);
};

#endif
