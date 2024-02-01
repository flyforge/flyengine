pl_requires(PL_CMAKE_PLATFORM_LINUX)

set (PL_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT OFF CACHE BOOL "Whether to add support for tracelogging via lttng.")
mark_as_advanced(FORCE PL_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)