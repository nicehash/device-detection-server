#pragma once
#ifdef _DEBUG
#    include <windows.h>
#    include <sstream>
#    define TRACE(x)                           \
     do {  std::wostringstream s;  s << "[" << __FILEW__ << ":" << __LINE__ << "]:" << x << '\n'; \
           OutputDebugString(s.str().c_str()); \
        } while(0)
#else
#  define TRACE(x)
#endif
