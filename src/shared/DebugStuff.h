#ifndef _DEBUGSTUFF_H
#define _DEBUGSTUFF_H


#ifdef _DEBUG
    #define DEBUG(code) {code;}
    #define DEBUG_APPENDIX " - DEBUG"
    #define CODEDEB(code) {fprintf(stderr,"[[ %s ]]\n",#code); code;}
#else
    #define DEBUG(code) /* code */
    #define DEBUG_APPENDIX
    #define CODEDEB(code) {code;}
#endif

#define NOT_REACHED_LINE { logerror("\nFILE '%s:%u' FUNC: '%s': How in the hell did you get here?!",__FILE__,__LINE__,__FUNCTION__); }



#endif
