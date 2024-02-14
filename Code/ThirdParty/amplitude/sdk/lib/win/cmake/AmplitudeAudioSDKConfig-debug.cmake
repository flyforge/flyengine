#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SparkyStudios::Audio::Amplitude" for configuration "Debug"
set_property(TARGET SparkyStudios::Audio::Amplitude APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(SparkyStudios::Audio::Amplitude PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/sdk/lib/win/Amplitude_d.lib"
  )

list(APPEND _cmake_import_check_targets SparkyStudios::Audio::Amplitude )
list(APPEND _cmake_import_check_files_for_SparkyStudios::Audio::Amplitude "${_IMPORT_PREFIX}/sdk/lib/win/Amplitude_d.lib" )

# Import target "SparkyStudios::Audio::amac" for configuration "Debug"
set_property(TARGET SparkyStudios::Audio::amac APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(SparkyStudios::Audio::amac PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/sdk/bin/win/amac.exe"
  )

list(APPEND _cmake_import_check_targets SparkyStudios::Audio::amac )
list(APPEND _cmake_import_check_files_for_SparkyStudios::Audio::amac "${_IMPORT_PREFIX}/sdk/bin/win/amac.exe" )

# Import target "SparkyStudios::Audio::samplerate" for configuration "Debug"
set_property(TARGET SparkyStudios::Audio::samplerate APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(SparkyStudios::Audio::samplerate PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/sdk/lib/win/samplerate_d.lib"
  )

list(APPEND _cmake_import_check_targets SparkyStudios::Audio::samplerate )
list(APPEND _cmake_import_check_files_for_SparkyStudios::Audio::samplerate "${_IMPORT_PREFIX}/sdk/lib/win/samplerate_d.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
