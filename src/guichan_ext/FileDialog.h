#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include "guichan.hpp"
#include "Panel.h"

class FileDialogCallback;
class VFSDir;

// a file dialog that works directly on the VFS
class FileDialog : public gcn::Window, gcn::ActionListener, gcn::ListModel,
    gcn::SelectionListener, gcn::KeyListener
{
public:

    FileDialog(); 
    ~FileDialog();
    void Open(bool save, const char *opr = NULL); // true: save, false: open
    void Close(void);
    std::string GetFileName(void);
    inline VFSFile *GetFile(void) { return _selFile; }
    std::string GetDirName(void);
    inline void SetCallback(FileDialogCallback *cb) { _callback = cb; }
    inline const char *GetOperation(void) { return _operation.c_str(); }
    inline bool IsSave(void) { return _save; }
    bool IsFileSelected(void);

    virtual std::string getElementAt(int i);
    virtual int getNumberOfElements(void);
    virtual void action(const gcn::ActionEvent& ae);
    virtual void valueChanged(const gcn::SelectionEvent& se);
    virtual void keyPressed(gcn::KeyEvent& ke);

protected:
    void _DoCallback(void);
    void _Update(void);
    void _HandleSelection(void);
    void _Descend(const std::string& subdir);
    void _Ascend(void);
    VFSDir *_GetCurrentVFSDir(void);
    bool _save;
    std::string _operation; // unique name to be able to identify the process later
    std::list<std::string> _dirstack;
    VFSFile *_selFile;
    FileDialogCallback *_callback;
    VFSDir *_curDir; // caches current dir, re-scan if NULL

    // GUI elements
    gcn::ListBox *lbFiles;
    gcn::ScrollArea *scroll;
    gcn::Button *bOk, *bCancel;
    gcn::Panel *pPreview;
    gcn::TextField *tbFilename;

};

class FileDialogCallback
{
public:
    virtual void FileChosenCallback(FileDialog *dlg) = 0;
};



#endif
