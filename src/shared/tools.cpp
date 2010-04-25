#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <fstream>
#include <errno.h>
#include "tools.h"

#if PLATFORM == PLATFORM_WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <mmsystem.h>
#else
#   include <sys/dir.h>
#   include <sys/stat.h>
#   include <sys/timeb.h>
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

void printchex(std::string in, bool spaces=true)
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

void printchex(char *in, uint32 len, bool spaces=true)
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

std::deque<std::string> GetFileList(std::string path)
{
    std::deque<std::string> files;

# ifndef _WIN32 // TODO: fix this function for linux if needed
    const char *p = path.c_str();
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(p);
    while (dirp)
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
            files.push_back(std::string(dp->d_name));
        else
            break;
    }
    if(dirp)
        closedir(dirp);

# else

    if(path.at(path.length()-1)!='/')
        path += "/";
    path += "*.*";
    const char *p = path.c_str();
    WIN32_FIND_DATA fil;
    HANDLE hFil=FindFirstFile(p,&fil);
    if(hFil!=INVALID_HANDLE_VALUE)
    {
        if( !(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            files.push_back(std::string(fil.cFileName));
        while(FindNextFile(hFil,&fil))
        {
            if( !(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                files.push_back(std::string(fil.cFileName));
        }
    }

# endif

    return files;
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
	bool result;
# ifdef _WIN32
	result = ::CreateDirectory(dir,NULL);
# else
	// NOT tested for Linux!! whats the return value on success?
	// TODO: fix me!
	result = mkdir(dir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	return result;
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
    return str;
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