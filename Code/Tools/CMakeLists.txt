

set (PLASMA_BUILD_TOOLS ON CACHE BOOL "Whether tool projects should be added to the solution")

pl_requires(PLASMA_BUILD_TOOLS)

pl_add_all_subdirs()