set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyPlModule
  PlTidyModule.cpp
  
  NameCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyPlModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )
