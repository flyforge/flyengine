######################################
### Jolt support
######################################

set (PL_3RDPARTY_JOLT_SUPPORT ON CACHE BOOL "Whether to add support for the Jolt physics engine.")
mark_as_advanced(FORCE PL_3RDPARTY_JOLT_SUPPORT)

######################################
### pl_requires_jolt()
######################################

macro(pl_requires_jolt)

	pl_requires(PL_3RDPARTY_JOLT_SUPPORT)

endmacro()
