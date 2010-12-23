#include "common.h"
#include "VFSFile.h"
#include "VFSDir.h"

VFSDir::~VFSDir()
{
    for(VFSFileMap::iterator it = _files.begin(); it != _files.end(); it++)
        it->second->ref--;
    for(VFSDirMap::iterator it = _subdirs.begin(); it != _subdirs.end(); it++)
        it->second->ref--;
}


bool VFSDir::add(VFSFile *f, bool overwrite /* = true */)
{
    VFSFileMap::iterator it = _files.find(f->name());
    
    if(it != _files.end())
    {
        if(overwrite)
        {
            VFSFile *oldf = it->second;
            if(oldf == f)
                return false;

            oldf->ref--;
            _files.erase(it);
        }
        else
            return false;
    }

    f->ref++;
    _files[f->name()] = f;
    return true;
}

bool VFSDir::merge(VFSDir *dir, bool overwrite /* = true */)
{
    bool result = false;
    for(VFSFileMap::iterator it = dir->_files.begin(); it != dir->_files.end(); it++)
        result = add(it->second, overwrite) || result;

    for(VFSDirMap::iterator it = dir->_subdirs.begin(); it != dir->_subdirs.end(); it++)
        result = insert(it->second, overwrite) || result;
    return result;
}

bool VFSDir::insert(VFSDir *subdir, bool overwrite /* = true */)
{
    VFSDirMap::iterator it = _subdirs.find(subdir->name());
    VFSDir *mydir;
    if(it != _subdirs.end())
        mydir = it->second;
    else
    {
        mydir = new VFSDir;
        mydir->_name = subdir->name();
        _subdirs[mydir->_name] = mydir;
    }

    return mydir->merge(subdir, overwrite);
}

VFSFile *VFSDir::getFile(const char *fn)
{
    char *slashpos = (char *)strchr(fn, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        *slashpos = 0; // temp change to avoid excess string mangling
        const char *sub = slashpos + 1;

        VFSDir *subdir = getDir(fn); // fn is null-terminated early here
        *slashpos = '/'; // restore original string

        return subdir ? subdir->getFile(sub) : NULL;
    }

    // no subdir? file must be in this dir now.
    VFSFileMap::iterator it = _files.find(fn);
    return it != _files.end() ? it->second : NULL;
}

VFSDir *VFSDir::getDir(const char *subdir, bool forceCreate /* = false */)
{
    VFSDir *ret = NULL;
    char *slashpos = (char *)strchr(subdir, '/');

    // if there is a '/' in the string, descend into subdir and continue there
    if(slashpos)
    {
        *slashpos = 0; // temp change to avoid excess string mangling
        const char *sub = slashpos + 1;

        VFSDirMap::iterator it = _subdirs.find(subdir);

        if(it != _subdirs.end())
        {
            *slashpos = '/'; // restore original string
            ret = it->second->getDir(sub, forceCreate); // descend into subdirs
        }
        else if(forceCreate)
        {
            VFSDir *ins = new VFSDir;
            ins->_name = subdir;
            *slashpos = '/'; // restore original string
            _subdirs[ins->_name] = ins;
            ret = ins->getDir(sub, true); // create remaining structure
        }
    }
    else
    {

        VFSDirMap::iterator it = _subdirs.find(subdir);
        if(it != _subdirs.end())
            ret = it->second;
        else if(forceCreate)
        {
            ret = new VFSDir;
            ret->_name = subdir;
            _subdirs[ret->_name] = ret;
        }
    }

    return ret;
}



// ----- VFSDirReal start here -----


VFSDirReal::VFSDirReal() : VFSDir()
{
}

uint32 VFSDirReal::load(const char *dir /* = NULL */)
{
    _abspath = dir;
    _name = _PathToFileName(dir); // path must not end with '/'
    std::deque<std::string> fl = GetFileList(dir);

    for(std::deque<std::string>::iterator it = fl.begin(); it != fl.end(); it++)
    {
        VFSFileReal *f = new VFSFileReal((_abspath + '/'  + *it).c_str());
        _files[f->name()] = f;
    }
    uint32 sum = fl.size();

    std::deque<std::string> dl = GetDirList(dir, false);
    
    for(std::deque<std::string>::iterator it = dl.begin(); it != dl.end(); it++)
    {
        VFSDirReal *d = new VFSDirReal;
        std::string full(dir);
        full += '/';
        full += *it;
        sum += d->load(full.c_str()); // GetDirList() always returns relative paths
        _subdirs[d->name()] = d;
    }
    return sum;
}
