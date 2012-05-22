#include "common.h"
#include "ResourceMgr.h"
#include "FileDialog.h"
#include "GuichanExt.h"


FileDialog::FileDialog()
: _selFile(NULL), _curDir(NULL)
{
    uint32 sx = 500;
    uint32 sy = 400;
    setMovable(true);
    setSize(sx, sy + gcn::Window::getTitleBarHeight());
    setFrameSize(2);
    setVisible(false);
    addKeyListener(this);

    // add GUI elements
    bOk = new gcn::Button();
    bOk->setSize(70, 40);
    bOk->addActionListener(this);
    bCancel = new gcn::Button("Cancel");
    bCancel->setSize(70, 40);
    bCancel->addActionListener(this);
    pPreview = new gcn::Panel(1, 1);
    pPreview->addActionListener(this);

    add(bCancel, sx - bCancel->getWidth() - 5, sy - bCancel->getHeight() - 5);
    int bx = bCancel->getX() - 5;
    int by = bCancel->getY() - bOk->getHeight() - 5;

    add(bOk, sx - bOk->getWidth() - 5, by);

    lbFiles = new gcn::ListBox2();
    lbFiles->setListModel(this);
    lbFiles->setWidth(bx);
    lbFiles->addSelectionListener(this);
    lbFiles->addActionListener(this);

    scroll = new gcn::ScrollArea(lbFiles);
    scroll->setSize(bx, by);
    scroll->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    scroll->setVerticalScrollAmount(3);
    add(scroll, 5, 5);

    tbFilename = new gcn::GreedyTextField();
    tbFilename->setSize(scroll->getWidth() - 10, 20);
    tbFilename->addActionListener(this);
    gcn::Window::add(tbFilename, scroll->getX(), scroll->getY() + scroll->getHeight() + 10);
}

FileDialog::~FileDialog()
{
    delete lbFiles;
    delete bOk;
    delete bCancel;
    delete pPreview;
    delete scroll;
}

void FileDialog::Open(bool save, const char *opr /* = NULL */)
{
    if(isVisible())
        return;
    _operation = opr ? opr : "";
    resMgr.vfs.Reload(true); // refresh eventually changed files
    setVisible(true);
    _save = save;
    bOk->setCaption(save ? "Save" : "Open");
    _Update();
}

void FileDialog::Close(void)
{
    gcn::Window::setVisible(false);
}

std::string FileDialog::GetDirName(void)
{
    std::string str;
    str.reserve(_dirstack.size() * 15); // guess size
    std::list<std::string>::iterator next;
    for(std::list<std::string>::iterator it = _dirstack.begin(); it != _dirstack.end(); it = next)
    {
        next = it;
        next++;
        const char *dn = it->c_str();
        str += dn;
        if(*dn && next != _dirstack.end()) // root is always empty string, so not put '/' after it
            str += '/';
    }
    return str;
}

std::string FileDialog::GetFileName(void)
{
    std::string fn(GetDirName());
    if(fn.length())
        fn += '/';
    fn += (_selFile ? _selFile->name() : tbFilename->getText());
    return fn;
}

void FileDialog::_Update(void)
{
    _curDir = NULL;
    std::string capt(_save ? "Save to..." : "Open file...");
    capt += " [";
    capt += GetDirName();
    capt += ']';
    gcn::Window::setCaption(capt);
}

void FileDialog::_Descend(const std::string& subdir)
{
    _dirstack.push_back(subdir);
    _Update();
}

void FileDialog::_Ascend(void)
{
    if(_dirstack.size())
    {
        _dirstack.pop_back();
        _Update();
    }
}

void FileDialog::_HandleSelection(void)
{
    _selFile = NULL;

    uint32 sel = lbFiles->getSelected();

    // user entered file manually, use that as name instead of _selFile
    if(tbFilename->getText().length() && getElementAt(sel) != tbFilename->getText())
    {
        _DoCallback();
        return;
    }

    if(sel == -1)
        return;
    else if(sel == 0)
    {
        _Ascend();
        return;
    }
    --sel; // skip [../]

    ttvfs::VFSDir *vd = _GetCurrentVFSDir();
    if(sel < vd->_subdirs.size())
    {
        ttvfs::DirIter it = vd->_subdirs.begin();
        advance(it, sel);
        _Descend(it->second->name());
    }
    else
    {
        sel -= vd->_subdirs.size();
        if(sel >= vd->_files.size())
        {
            logerror("FileDialog::_HandleSelection: sel=%u, vd->_files.size()=%u. HUH??", sel, vd->_files.size());
            return; // oops?!
        }
        ttvfs::FileIter it = vd->_files.begin();
        advance(it, sel);
        _selFile = it->second;

        _DoCallback();
    }

}

ttvfs::VFSDir *FileDialog::_GetCurrentVFSDir(void)
{
    if(!_curDir)
    {
        ttvfs::VFSDir *vd = resMgr.vfs.GetDirRoot();
        ttvfs::VFSDir *next;
        for(std::list<std::string>::iterator it = _dirstack.begin(); it != _dirstack.end(); ++it)
        {
            next = vd->getDir(it->c_str());
            if(!next)
                break;
            vd = next;
        }
        _curDir = vd;
    }
    return _curDir;
}

void FileDialog::_DoCallback(void)
{
    if(_callback)
        _callback->FileChosenCallback(this);
    else
        Close();
}

bool FileDialog::IsFileSelected(void)
{
    int i =  lbFiles->getSelected();
    return i >= int(_GetCurrentVFSDir()->_subdirs.size()) + 1; // +1 because of [../] at index 0
}

// -- from gcn::ListModel --

std::string FileDialog::getElementAt(int i)
{
    if(!i)
        return "[../]";
    --i;
    // directories first, then files
    ttvfs::VFSDir *vd = _GetCurrentVFSDir();
    if(uint32(i) < vd->_subdirs.size())
    {
        ttvfs::DirIter it = vd->_subdirs.begin();
        advance(it, i);
        std::string name("[");
        name += it->second->name();
        name += "/]";
        return name;
    }
    else
    {
        i -= vd->_subdirs.size();
        if(uint32(i) >= vd->_files.size())
            return "";
        ttvfs::FileIter it = vd->_files.begin();
        advance(it, i);
        return it->second->name();
    }
}

int FileDialog::getNumberOfElements(void)
{
    ttvfs::VFSDir *cur = _GetCurrentVFSDir();
    return cur->_subdirs.size() + cur->_files.size() + 1; // +1 because of [../] at index 0
}

// -- from gcn::ActionListener --

void FileDialog::action(const gcn::ActionEvent& ae)
{
    gcn::Widget *src = ae.getSource();

    if(src == bOk || src == lbFiles || src == tbFilename)
    {
        _HandleSelection();
    }
    else if(src == bCancel)
    {
        Close();
    }
}

// -- from gcn::SelectionListener --

void FileDialog::valueChanged(const gcn::SelectionEvent& se)
{
    gcn::Widget *src = se.getSource();
    
    if(src == lbFiles)
    {
        if(IsFileSelected())
            tbFilename->setText(getElementAt(lbFiles->getSelected()));
        else
            tbFilename->setText(""); // do not put directory name in text field if directory selected

        // TODO: update preview, once implemented
    }
}

// -- from gcn::KeyListener --

void FileDialog::keyPressed(gcn::KeyEvent& ke)
{
    if(ke.getKey() == gcn::Key::ESCAPE)
    {
        Close();
    }
    else if(ke.getKey() == gcn::Key::ENTER)
    {
        _HandleSelection();
    }
}
