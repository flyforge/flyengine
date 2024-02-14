# #####################################
# ## Optick Profiler support
# #####################################

set (PL_BUILD_OPTICK ON CACHE BOOL "Whether support for the Optick profiler should be added")

# #####################################
# ## pl_requires_optick()
# #####################################

macro(pl_requires_optick)
  pl_requires(PL_BUILD_OPTICK)
endmacro()