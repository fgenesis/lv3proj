#ifndef _TOOLS_H
#define _TOOLS_H

#include "common.h"

void mtRandSeed(uint32 seed);
int32 irand(int32 min, int32 max);
uint32 urand(uint32 min, uint32 max);
int32 rand32(void);
double rand_norm(void);
double rand_chance(void);
void printchex(const std::string&,bool);
void printchex(const char *in, uint32 len, bool);
std::string stringToUpper(std::string);
std::string stringToLower(std::string);
uint64 toInt(std::string);
std::string toHexDump(uint8* array,uint32 size,bool spaces=true,uint32 per_line=0);
std::deque<std::string> GetFileList(std::string);
std::deque<std::string> GetDirList(std::string, bool recursive = false);
bool FileExists(std::string);
bool IsDirectory(const char *);
bool CreateDir(const char*);
bool CreateDirRec(const char*);
uint32 getMSTime(void);
uint32 getMSTimeDiff(uint32, uint32);
uint32 GetFileSize(const char*);
void _FixFileName(std::string&);
std::string _PathToFileName(std::string);
std::string _PathStripLast(std::string);
std::string FileGetExtension(std::string str, bool withDot = true);
std::string NormalizeFilename(std::string);
std::string AddPathIfNecessary(std::string file, std::string path);
uint32 HexStrToInt(const char*);
uint32 fastintpow(uint32 base, uint32 power);
uint64 fastintpow64(uint64 base, uint32 power);
std::string GetTimeString(void);
std::string GetDateString(void);
std::string GetProgramDir(void); // file path of the exe/binary; this is NOT the working directory.
bool SetWorkingDir(std::string);
void HexStrToByteArray(uint8 *dst, const char *str);
std::string FixMultiSlashes(const std::string& s);
void MakeSlashTerminated(std::string& s);
void GetFileListRecursive(const std::string dir, std::list<std::string>& files, bool withQueriedDir = false);
bool WildcardMatch(const char *str, const char *pattern);
uint32 GetConsoleWidth(void);


void SplitFilenameToProps(const char *in, std::string *fn = NULL, std::string *s1 = NULL, 
                                 std::string *s2 = NULL,  std::string *s3 = NULL,  std::string *s4 = NULL,
                                 std::string *s5 = NULL);

template <class T> void StrSplit(const std::string &src, const std::string &sep, T& container, bool keepEmpty = false)
{
    std::string s;
    for (std::string::const_iterator i = src.begin(); i != src.end(); i++)
    {
        if (sep.find(*i) != std::string::npos)
        {
            if (keepEmpty || s.length())
                container.push_back(s);
            s = "";
        }
        else
        {
            s += *i;
        }
    }
    if (keepEmpty || s.length())
        container.push_back(s);
}

inline float fastabs(float f)
{
    int i = ((*(int*)&f) & 0x7fffffff);
    return (*(float*)&i);
}

inline float fastneg(float f)
{
    int i = ((*(int*)&f) ^ 0x80000000);
    return (*(float*)&i);
}

inline int fastsgn(float f)
{
    return 1 + (((*(int*)&f) >> 31) << 1);
}

inline bool fastsgncheck(float f)
{
    return (*(int*)&f) & 0x80000000;
}

inline int32 int32r(float f)
{
    return int32(f + 0.5f);
}

// floor to next power of 2
inline uint32 flp2(uint32 x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x - (x >> 1);
}

// ceil to next power of 2
inline uint32 clp2(uint32 x)
{
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

// helps to convert strings to other types, or from other types to a string
template <typename in_value, typename out_value>
inline void convert(const in_value &ival, out_value &oval)
{
    std::stringstream sstream;
    sstream << ival; // insert value in stream
    sstream >> oval; // get value from stream
}

template <typename T> class AutoPtrVector
{
public:
    AutoPtrVector(uint32 prealloc) :v(prealloc)
    {
        for(uint32 i = 0; i < prealloc; ++i)
            v[i] = NULL;
    }
    ~AutoPtrVector()
    {
        for(uint32 i = 0; i < v.size(); ++i)
            if(v[i])
                delete v[i];
    }
    std::vector<T*> v;
};

#endif
