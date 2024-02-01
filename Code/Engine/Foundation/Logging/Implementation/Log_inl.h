#pragma once

#if PL_DISABLED(PL_COMPILE_FOR_DEVELOPMENT)

inline void plLog::Dev(plLogInterface* /*pInterface*/, const plFormatString& /*string*/) {}

#endif

#if PL_DISABLED(PL_COMPILE_FOR_DEBUG)

inline void plLog::Debug(plLogInterface* /*pInterface*/, const plFormatString& /*string*/) {}

#endif
