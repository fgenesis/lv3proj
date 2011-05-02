#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <fstream>
#include <cerrno>
#include <stack>
#include "tools.h"

#if PLATFORM == PLATFORM_WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <mmsystem.h>
#   include <direct.h>
#else
#   include <sys/dir.h>
#   include <sys/stat.h>
#   include <sys/timeb.h>
#   include <sys/types.h>
#   include <sys/ioctl.h>
#   include <unistd.h>
#endif

#include "MersenneTwister.h"
static MTRand mtRand;

void mtRandSeed(uint32 seed)
{
    mtRand.seed(seed);
}

int32 irand (int32 min, int32 max)
{
    return int32 (mtRand.randInt (max - min)) + min;
}

uint32 urand (uint32 min, uint32 max)
{
    return mtRand.randInt (max - min) + min;
}

int32 rand32 ()
{
    return mtRand.randInt ();
}

double rand_norm(void)
{
    return mtRand.randExc ();
}

double rand_chance (void)
{
    return mtRand.randExc (100.0);
}

void printchex(const std::string& in, bool spaces=true)
{
	unsigned int len=0,i;
    len=in.length();
	printf("[");
	if(spaces)
		for(i=0;i<len;i++)printf("%x ",(unsigned char)in[i]);
	else
		for(i=0;i<len;i++)printf("%x",(unsigned char)in[i]);
	printf("]\n");
}

void printchex(const char *in, uint32 len, bool spaces=true)
{
	unsigned int i;
	printf("[");
	if(spaces)
		for(i=0;i<len;i++)printf("%x ",(unsigned char)in[i]);
	else
		for(i=0;i<len;i++)printf("%x",(unsigned char)in[i]);
	printf("]\n");
}

std::string stringToLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s;
}

std::string stringToUpper(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), toupper);
	return s;
}

uint64 toInt(std::string str)
{
    if(str.empty())
        return 0;
    str = stringToUpper(str);
    if(str.length() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        return strtoul(&(str.c_str()[2]),NULL,16);
    else
        return strtoul(str.c_str(),NULL,10);
}

std::string toHexDump(uint8* array, uint32 size, bool spaces, uint32 per_line)
{
    std::stringstream ss;
    char buf[5];
    for(uint32 i=0;i<size;i++)
    {
        if(array[i])
        {
            sprintf(buf,(array[i]<=0x0F)?"0%X":"%X",(uint32)array[i]);
            ss << buf;
        }
        else
            ss << "00"; // little hacklike fix

        if(per_line && !((i+1) % per_line))
        {
            ss << "\n";
            continue;
        }

        if(spaces)
            ss << ' ';
    }
    return ss.str();
}

// returns list of *plain* file names in given directory,
// without paths, and without anything else
std::deque<std::string> GetFileList(std::string path)
{
    std::deque<std::string> files;
# ifndef _WIN32
    const char *p = path.c_str();
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(p);
    if(dirp)
    {
        while((dp=readdir(dirp)) != NULL)
        {
            if (dp->d_type != DT_DIR) // only add if it is not a directory
            {
                std::string s(dp->d_name);
                files.push_back(s);
            }
        }
        closedir(dirp);
    }
        
# else

    if(path[path.length()-1] != '/')
        path += "/";
    WIN32_FIND_DATA fil;
    std::string search = path + "*";
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil!=INVALID_HANDLE_VALUE)
    {
        do
        {
            if(!(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string s(fil.cFileName);
                files.push_back(s);
            }
        }
        while(FindNextFile(hFil, &fil));

        FindClose(hFil);
    }

# endif

    return files;
}

// returns a list of directory names in the given directory, *without* the source dir.
// if getting the dir list recursively, all paths are added, except *again* the top source dir beeing queried.
std::deque<std::string> GetDirList(std::string path, bool recursive /* = false */)
{
    std::deque<std::string> dirs;

#ifndef _WIN32

    const char *p = path.c_str();
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(p);
    if(dirp)
    {
        while(dp = readdir(dirp)) // assignment is intentional
        {
            if (dp->d_type == DT_DIR) // only add if it is a directory
            {
                if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
                {
                    std::string s = dp->d_name;
				    dirs.push_back(s);
                    if (recursive) // needing a better way to do that
                    {
                        std::deque<std::string> newdirs = GetDirList(s);
                        for(std::deque<std::string>::iterator it = newdirs.begin(); it != newdirs.end(); ++it)
                        {
                            std::string d = s + *it;
                            dirs.push_back(d);
                        }
                    }
                }
            }
        }
        closedir(dirp);
    }

#else

    if(path[path.length()-1] != '/')
        path += "/";

    std::string search = path + "*";
    WIN32_FIND_DATA fil;
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil!=INVALID_HANDLE_VALUE)
    {
        do
        {
            if( fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if (!strcmp(fil.cFileName, ".") || !strcmp(fil.cFileName, ".."))
                    continue;
                std::string s = fil.cFileName;
                dirs.push_back(s);

                if (recursive) // needing a better way to do that
                {
                    std::deque<std::string> newdirs = GetDirList(s);
                    for(std::deque<std::string>::iterator it = newdirs.begin(); it != newdirs.end(); ++it)
                    {
                        std::string d = s + *it;
                        dirs.push_back(d);
                    }
                }
            }
        }
        while(FindNextFile(hFil, &fil));

        FindClose(hFil);
    }

#endif

    return dirs;
}

bool FileExists(std::string fn)
{
	std::fstream f;
	f.open(fn.c_str(),std::ios_base::in);
	if (f.is_open())
	{
		f.close();
		return true;
	}
	return false;
}

// must return true if creating the directory was successful
bool CreateDir(const char *dir)
{
    if(IsDirectory(dir))
        return true;
	bool result;
# ifdef _WIN32
	result = ::CreateDirectory(dir,NULL);
# else
	result = !mkdir(dir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	return result;
}

bool CreateDirRec(const char *dir)
{
    if(IsDirectory(dir))
        return true;
    bool result = true;
    std::list<std::string> li;
    StrSplit(dir, "/\\", li, false);
    std::string d;
    d.reserve(strlen(dir));
    bool last;
    for(std::list<std::string>::iterator it = li.begin(); it != li.end(); it++)
    {
        d += *it;
        last = CreateDir(d.c_str());
        result = last && result;
        d += '/';
    }
    return result || last;
}

// current system time in ms
uint32 getMSTime(void)
{
    uint32 time_in_ms = 0;
#if PLATFORM == PLATFORM_WIN32
    time_in_ms = timeGetTime();
#else
    struct timeb tp;
    ftime(&tp);

    time_in_ms = tp.time * 1000 + tp.millitm;
#endif

    return time_in_ms;
}

uint32 getMSTimeDiff(uint32 oldMSTime, uint32 newMSTime)
{
    // getMSTime() have limited data range and this is case when it overflow in this tick
    if (oldMSTime > newMSTime)
        return (0xFFFFFFFF - oldMSTime) + newMSTime;
    else
        return newMSTime - oldMSTime;
}

uint32 GetFileSize(const char* sFileName)
{
    if(!sFileName || !*sFileName)
        return 0;
    std::ifstream f;
    f.open(sFileName, std::ios_base::binary | std::ios_base::in);
    if (!f.good() || f.eof() || !f.is_open()) { return 0; }
    f.seekg(0, std::ios_base::beg);
    std::ifstream::pos_type begin_pos = f.tellg(), end_pos;
    f.seekg(0, std::ios_base::end);
    end_pos = f.tellg();
    f.close();
    return end_pos - begin_pos;
}

// fix filenames for linux ( '/' instead of windows '\')
void _FixFileName(std::string& str)
{
    for(uint32 i = 0; i < str.length(); i++)
        if(str[i]=='\\')
            str[i]='/';
}

// extracts the file name from a given path
std::string _PathToFileName(std::string str)
{
    uint32 pathend = str.find_last_of("/\\");
    if(pathend != std::string::npos)
    {
        return str.substr(pathend+1);
    }
    return str;
}

// extracts the file name from a given path
std::string FileGetExtension(std::string str, bool withDot /* = true */)
{
    uint32 dot = str.rfind('.');
    if(dot != std::string::npos)
    {
        return str.substr(dot + (withDot ? 0 : 1));
    }
    return "";
}


// strips a file name from a given path, or the last folder if the path doesnt end with '/' or '\'
std::string _PathStripLast(std::string str)
{
    uint32 pathend = str.find_last_of("/\\");
    if(pathend != std::string::npos)
    {
        return str.substr(0, pathend + 1);
    }
    return ""; // no / or \, strip all
}

std::string NormalizeFilename(std::string s)
{
    uint32 p;
    while( (p = s.find('\\')) != std::string::npos)//Replace \ by /
    {
        s.replace(p,1,"/");
    }
    while( (p = s.find(' ')) != std::string::npos)//Replace space by _
    {
        s.replace(p,1,"_");
    }
    std::transform(s.begin(), s.end(), s.begin(), tolower);
    return s;
}

std::string AddPathIfNecessary(std::string fn, std::string path)
{
    // if a '/' or '\' is found as first char, treat the path as absolute (or relative to working dir or whatever)
    // otherwise, fix path if necessary and append file name
    if(fn.length() && (fn[0] == '\\' || fn[0] =='/'))
        return fn;

    uint32 pathlen = path.length();
    bool endslash = path[pathlen - 1] == '/';
    std::string fixpath(path);
    if(!endslash)
        fixpath += '/';
    std::string fullfn(fixpath);
    fullfn += fn;
    return fullfn;
}

uint32 HexStrToInt(const char *str)
{
    return strtoul(str, NULL, 16);
}

uint32 fastintpow(uint32 base, uint32 power)
{
    if(power > 1)
    {
        if (power % 2)
            return base * fastintpow(base * base, power / 2);
        else
            return fastintpow(base * base, power / 2);
    }
    else if(power == 1)
        return base;

    return 1;
}

uint64 fastintpow64(uint64 base, uint32 power)
{
    if(power > 1)
    {
        if (power % 2)
            return base * fastintpow64(base * base, power / 2);
        else
            return fastintpow64(base * base, power / 2);
    }
    else if(power == 1)
        return base;

    return 1;
}

std::string GetDateString(void)
{
    time_t t = time(NULL);
    tm* aTm = localtime(&t);
    char str[30];
    //       YYYY   year
    //       MM     month (2 digits 01-12)
    //       DD     day (2 digits 01-31)
    //       HH     hour (2 digits 00-23)
    //       MM     minutes (2 digits 00-59)
    //       SS     seconds (2 digits 00-59)
    sprintf(str,"%-4d-%02d-%02d %02d:%02d:%02d ",aTm->tm_year+1900,aTm->tm_mon+1,aTm->tm_mday,aTm->tm_hour,aTm->tm_min,aTm->tm_sec);
    return std::string(str);
}

std::string GetTimeString(void)
{
    time_t t = time(NULL);
    tm* aTm = localtime(&t);
    char str[15];
    sprintf(str,"%02d:%02d:%02d", aTm->tm_hour,aTm->tm_min,aTm->tm_sec);
    return std::string(str);
}

// splits string like "filename.anim:4:default" or "image.png:32:32:16:16". leaves unused args unchanged!
void SplitFilenameToProps(const char *in, std::string *fn, std::string *s1 /* = NULL */,
                                     std::string *s2 /* = NULL */, std::string *s3 /* = NULL */, std::string *s4 /* = NULL */,
                                     std::string *s5 /* = NULL */)
{
    std::vector<std::string> fields;
    StrSplit(in, ":", fields, true);
    if(fields.size() >= 1 && fn)
        *fn = fields[0];
    if(fields.size() >= 2 && s1)
        *s1 = fields[1];
    if(fields.size() >= 3 && s2)
        *s2 = fields[2];
    if(fields.size() >= 4 && s3)
        *s3 = fields[3];
    if(fields.size() >= 5 && s4)
        *s4 = fields[4];
    if(fields.size() >= 6 && s5)
        *s5 = fields[5];
}

// taken from a post from http://www.gamedev.net/topic/459511-something-like-getmodulefilename-in-linux/
std::string GetProgramDir(void)
{
#if PLATFORM == PLATFORM_WIN32
    char szPath[1024];
    uint32 len = GetModuleFileName( NULL, szPath, 1024 );
    if(!len)
        return "";
    std::string path(szPath);
    std::string::size_type t = path.find_last_of("/\\");
    path = path.substr(0,t);
    _FixFileName(path);
    return path;

#elif PLATFORM == PLATFORM_UNIX

    std::stringstream ss;
    ss << "/proc/" << getpid() << "/exe";
    char proc[1024];
    int ch = readlink(ss.str().c_str(), proc, 1024);
    std::string path;
    if (ch != -1)
    {
        proc[ch] = 0;
        path = proc;
        std::string::size_type t = path.find_last_of("/");
        path = path.substr(0,t);
    }
    return path;

#elif PLATFORM == PLATFORM_APPLE
    std::string path = "./";
    ProcessSerialNumber PSN;
    ProcessInfoRec pinfo;
    FSSpec pspec;
    FSRef fsr;
    OSStatus err;
    /* set up process serial number */
    PSN.highLongOfPSN = 0;
    PSN.lowLongOfPSN = kCurrentProcess;
    /* set up info block */
    pinfo.processInfoLength = sizeof(pinfo);
    pinfo.processName = NULL;
    pinfo.processAppSpec = &pspec;
    /* grab the vrefnum and directory */
    err = GetProcessInformation(&PSN, &pinfo);
    if (!err)
    {
        char c_path[2048];
        FSSpec fss2;
        int tocopy;
        err = FSMakeFSSpec(pspec.vRefNum, pspec.parID, 0, &fss2);
        if (!err)
        {
            err = FSpMakeFSRef(&fss2, &fsr);
            if (!err)
            {
                char c_path2[2049];
                err = (OSErr)FSRefMakePath(&fsr, (UInt8*)c_path2, 2048);
                if (!err)
                {
                    path = c_path2;
                }
            }
        }
    }
    return path;

#endif
}

bool SetWorkingDir(std::string d)
{
#if PLATFORM == PLATFORM_WIN32
    return !_chdir(d.c_str());
#elif PLATFORM == PLATFORM_WIN32 || PLATFORM == PLATFORM_APPLE
    return !chdir(d.c_str());
#endif
}

void HexStrToByteArray(uint8 *dst, const char *str)
{
    int l = strlen(str);
    char a, b;
    int hi, lo;
    // uneven digit count? treat as if there was another '0' char in front
    if(l & 1)
    {
        a = '0';
        b = *str++;
    }
    l /= 2; // if uneven, this rounds down correctly
    
    for(int i=0; i < l; i++)
    {
        a = *str++;
        b = *str++;

        if(isdigit(a))            hi = a - '0';
        else if(a>='A' && a<='F') hi = a - 'A' + 10;
        else if(a>='a' && a<='f') hi = a - 'a' + 10;
        else                      hi = 0;

        if(isdigit(b))            lo = b - '0';
        else if(b>='A' && b<='F') lo = b - 'A' + 10;
        else if(b>='a' && b<='f') lo = b - 'a' + 10;
        else                      lo = 0;

        dst[i] = (hi << 4) + lo;
    }
}

std::string FixMultiSlashes(const std::string& s)
{
    std::string r;
    r.reserve(s.length() + 1);
    char last = 0, cur;
    for(uint32 i = 0; i < s.length(); ++i)
    {
        cur = s[i];
        if(last == '/' && cur == '/')
            continue;
        r += cur;
        last = cur;
    }
    return r;
}


bool IsDirectory(const char *s)
{
#if PLATFORM == PLATFORM_WIN32
    DWORD dwFileAttr = GetFileAttributes(s);
    if(dwFileAttr == INVALID_FILE_ATTRIBUTES)
        return false;
    return dwFileAttr & FILE_ATTRIBUTE_DIRECTORY;
#else
    if ( access( s, 0 ) == 0 )
    {
        struct stat status;
        stat( s, &status );
        return status.st_mode & S_IFDIR;
    }
    return false;
#endif
}

void MakeSlashTerminated(std::string& s)
{
    if(s.length() && s[s.length() - 1] != '/')
        s += '/';
}

void GetFileListRecursive(std::string dir, std::list<std::string>& files, bool withQueriedDir /* = false */)
{
    std::stack<std::string> stk;

    if(withQueriedDir)
    {
        stk.push(dir);
        while(stk.size())
        {
            dir = stk.top();
            stk.pop();
            MakeSlashTerminated(dir);
            
            std::deque<std::string> fl = GetFileList(dir);
            for(std::deque<std::string>::iterator fit = fl.begin(); fit != fl.end(); ++fit)
                files.push_back(dir + *fit);

            std::deque<std::string> dirlist = GetDirList(dir, true);
            for(std::deque<std::string>::iterator it = dirlist.begin(); it != dirlist.end(); ++it)
                stk.push(dir + *it);
        }
    }
    else
    {
        std::string topdir = dir;
        MakeSlashTerminated(topdir);
        stk.push("");
        while(stk.size())
        {
            dir = stk.top();
            stk.pop();
            MakeSlashTerminated(dir);

            std::deque<std::string> fl = GetFileList(topdir + dir);
            for(std::deque<std::string>::iterator fit = fl.begin(); fit != fl.end(); ++fit)
                files.push_back(dir + *fit);

            std::deque<std::string> dirlist = GetDirList(topdir + dir, true);
            for(std::deque<std::string>::iterator it = dirlist.begin(); it != dirlist.end(); ++it)
                stk.push(dir + *it);
        }
    }
}

// from http://board.byuu.org/viewtopic.php?f=10&t=1089&start=15
bool WildcardMatch(const char *str, const char *pattern)
{
    const char *cp = 0, *mp = 0;
    while(*str && *pattern != '*')
    {
        if(*pattern != *str && *pattern != '?')
            return false;
        pattern++, str++;
    }

    while(*str)
    {
        if(*pattern == '*')
        {
            if(!*++pattern)
                return 1;
            mp = pattern;
            cp = str + 1;
        }
        else if(*pattern == *str || *pattern == '?')
        {
            ++pattern;
            ++str;
        }
        else
        {
            pattern = mp;
            str = cp++;
        }
    }

    while(*pattern++ == '*');

    return !*pattern;
}

uint32 GetConsoleWidth(void)
{
#if PLATFORM == PLATFORM_WIN32
    HANDLE hOut;
    CONSOLE_SCREEN_BUFFER_INFO SBInfo;
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hOut,
        &SBInfo);
    return SBInfo.dwSize.X;
#else
    struct winsize ws;
    if (ioctl(0,TIOCGWINSZ,&ws))
        return 80; // the standard, because we don't know any better
    return ws.ws_col;
#endif
}

// copy strings, mangling newlines to system standard
// windows has 13+10
// *nix has 10
size_t strnNLcpy(char *dst, const char *src, uint32 n /* = -1 */)
{
    char *olddst = dst;
    bool had10 = false, had13 = false;

    --n; // reserve 1 for \0 at end

    while(*src && n)
    {
        if((had13 && *src == 10) || (had10 && *src == 13))
        {
            ++src; // last was already mangled
            had13 = had10 = false; // processed one CRLF pair
            continue;
        }
        had10 = *src == 10;
        had13 = *src == 13;

        if(had10 || had13)
        {
            *dst++ = '\n';
            ++src;
        }
        else
            *dst++ = *src++;

        --n;
    }

    *dst++ = 0;

    return dst - olddst;
}
