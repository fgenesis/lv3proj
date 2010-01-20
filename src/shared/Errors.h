#ifndef ERRORS_H
#define ERRORS_H

#include "common.h"
#include "assert.h"

#define WPAssert( assertion ) { if (!(assertion)) { fprintf( stderr, "\n%s:%i in %s ASSERTION FAILED2:\n  %s\n", __FILE__, __LINE__,__FUNCTION__,  #assertion); assert( #assertion &&0 ); } }
#define WPError( assertion, errmsg ) if( ! (assertion) ) { logerror( "%\n%s:%i in %s ERROR:\n  %s\n", __FILE__, __LINE__, __FUNCTION__, (char *)errmsg ); assert( false ); }
#define WPWarning( assertion, errmsg ) if( ! (assertion) ) { logerror( "\n%s:%i in %s WARNING:\n  %s\n", __FILE__, __LINE__, __FUNCTION__, (char *)errmsg ); }
#define WPWarningAssert(assertion) if( ! (assertion) ) { logerror( "\n%s:%i in %s WARNING:\n  %s\n", __FILE__, __LINE__, __FUNCTION__, #assertion ); }
#define WPFatal( assertion, errmsg ) if( ! (assertion) ) { logerror( "\n%s:%i in %s FATAL ERROR:\n  %s\n", __FILE__, __LINE__, __FUNCTION__, (char *)errmsg ); assert( #assertion &&0 ); abort(); }
#define ASSERT WPAssert

#endif
