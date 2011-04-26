#ifndef _LOG_H
#define _LOG_H

enum Color
{
    BLACK,
    RED,
    GREEN,
    BROWN,
    BLUE,
    MAGENTA,
    CYAN,
    GREY,
    YELLOW,
    LRED,
    LGREEN,
    LBLUE,
    LMAGENTA,
    LCYAN,
    WHITE
};

void log_prepare(const char *fn, const char *mode);
void log_setloglevel(uint8 lvl);
void log_setlogtime(bool b);
void log(const char *str, ...) ATTR_PRINTF(1,2);
void logdetail(const char *str, ...) ATTR_PRINTF(1,2);
void logdebug(const char *str, ...) ATTR_PRINTF(1,2);
void logdev(const char *str, ...) ATTR_PRINTF(1,2);
void logerror(const char *str, ...) ATTR_PRINTF(1,2);
void logcritical(const char *str, ...) ATTR_PRINTF(1,2);
void logcustom(uint8 loglevel, Color color, const char *str, ...) ATTR_PRINTF(3,4);
void log_close();
void _log_setcolor(bool,Color);
void _log_resetcolor(bool);


const int Color_count = int(WHITE)+1;

#ifdef _DEBUG
#  define DEBUG_LOG logdebug
#else
#  define DEBUG_LOG
#endif

#endif
