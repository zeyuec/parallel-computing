#ifdef DEBUG_BUILD
#define DBGMSG(os, msg) \
        (os) << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
        << msg << std::endl
#else
#define DBGMSG(os, msg) 
#endif
