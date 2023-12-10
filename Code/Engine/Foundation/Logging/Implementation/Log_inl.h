#pragma once

#if PLASMA_DISABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)

inline void plLog::Dev(plLogInterface* /*pInterface*/, const plFormatString& /*string*/) {}

#endif

#if PLASMA_DISABLED(PLASMA_COMPILE_FOR_DEBUG)

inline void plLog::Debug(plLogInterface* /*pInterface*/, const plFormatString& /*string*/) {}

#endif
