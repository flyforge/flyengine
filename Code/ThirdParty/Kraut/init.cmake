######################################
### Jolt support
######################################

set (PLASMA_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE PLASMA_3RDPARTY_KRAUT_SUPPORT)

######################################
### pl_requires_kraut()
######################################

macro(pl_requires_kraut)

	pl_requires(PLASMA_3RDPARTY_KRAUT_SUPPORT)

endmacro()
