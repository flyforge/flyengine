### Assimp
set (PLASMA_3RDPARTY_ASSIMP_SUPPORT ON CACHE BOOL "Whether to add support for the asset importer.")
mark_as_advanced(FORCE PLASMA_3RDPARTY_ASSIMP_SUPPORT)

set (PLASMA_3RDPARTY_ASSIMP_DRACO_SUPPORT OFF CACHE BOOL "Whether to include Draco mesh compression support in AssImp.")
mark_as_advanced(FORCE PLASMA_3RDPARTY_ASSIMP_DRACO_SUPPORT)

macro(pl_requires_assimp)
	
	pl_requires(PLASMA_3RDPARTY_ASSIMP_SUPPORT)
	pl_requires_one_of(PLASMA_CMAKE_PLATFORM_LINUX PLASMA_CMAKE_PLATFORM_WINDOWS_DESKTOP)

endmacro()
