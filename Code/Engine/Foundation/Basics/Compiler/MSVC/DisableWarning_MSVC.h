
#ifdef PLASMA_MSVC_WARNING_NUMBER

#  if PLASMA_ENABLED(PLASMA_COMPILER_MSVC)

#    pragma warning(push)
#    pragma warning(disable \
                    : PLASMA_MSVC_WARNING_NUMBER)

#  endif

#  undef PLASMA_MSVC_WARNING_NUMBER

#endif
