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

#ifdef _DEBUG
#    define DEBUG_ASSERT_RETURN(a,r) { ASSERT(a); }
#    define DEBUG_ASSERT_RETURN_VOID(a) { ASSERT(a); }
#else
#    define DEBUG_ASSERT_RETURN(a,r) { if(!(a)) return (r); }
#    define DEBUG_ASSERT_RETURN_VOID(a) { if(!(a)) return; }
#endif



#endif
