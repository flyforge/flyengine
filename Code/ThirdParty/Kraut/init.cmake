######################################
### Jolt support
######################################

set (PL_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE PL_3RDPARTY_KRAUT_SUPPORT)

######################################
### pl_requires_kraut()
######################################

macro(pl_requires_kraut)

	pl_requires(PL_3RDPARTY_KRAUT_SUPPORT)

endmacro()
