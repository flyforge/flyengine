set (PLASMA_BUILD_UNITTESTS ON CACHE BOOL "Whether unit-test projects should be added to the solution")

pl_requires(PLASMA_BUILD_UNITTESTS)

pl_add_all_subdirs()