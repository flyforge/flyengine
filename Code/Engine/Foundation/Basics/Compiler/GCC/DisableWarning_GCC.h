
#ifdef PLASMA_GCC_WARNING_NAME

#  if PLASMA_ENABLED(PLASMA_COMPILER_GCC)

#    pragma GCC diagnostic push
_Pragma(PLASMA_STRINGIZE(GCC diagnostic ignored PLASMA_GCC_WARNING_NAME))

#  endif

#  undef PLASMA_GCC_WARNING_NAME

#endif
