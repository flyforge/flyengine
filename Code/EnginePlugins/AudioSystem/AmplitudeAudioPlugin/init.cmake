set (PL_3RDPARTY_AMPLITUDE_SUPPORT ON CACHE BOOL "Whether to add support for the Amplitude Audio Engine.")
mark_as_advanced(FORCE PL_3RDPARTY_AMPLITUDE_SUPPORT)

# Amplitude installation path
set(PL_AMPLITUDE_SDK_PATH "${PL_SOURCE_DIR}/ThirdParty/amplitude" CACHE PATH "The path to Amplitude Audio SDK libraries.")

# Check for a known file in the SDK path to verify the path
function(is_valid_sdk sdk_path is_valid)
    set(${is_valid} FALSE PARENT_SCOPE)
    if(EXISTS ${sdk_path})
        set(sdk_file ${sdk_path}/sdk/include/SparkyStudios/Audio/Amplitude/Amplitude.h)
        if(EXISTS ${sdk_file})
            set(${is_valid} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Paths that will be checked, in order:
# - CMake cache variable
# - A Environment Variable
set(AMPLITUDE_SDK_PATHS
    "${PL_AMPLITUDE_SDK_PATH}"
    "$ENV{PL_AMPLITUDE_ROOT_PATH}"
)

set(found_sdk FALSE)
foreach(candidate_path ${AMPLITUDE_SDK_PATHS})
    is_valid_sdk(${candidate_path} found_sdk)
    if(found_sdk)
        # Update the Amplitude installation path variable internally
        set(PL_AMPLITUDE_SDK_PATH "${candidate_path}")
        break()
    endif()
endforeach()

if(NOT found_sdk)
    # If we don't find a path that appears to be a valid Amplitude install, we can bail here.
    # No 3rdParty::AmplitudeAudioSDK target will exist, so that can be checked elsewhere.
    return()
endif()

message(STATUS "Using Amplitude Audio SDK at ${PL_AMPLITUDE_SDK_PATH}")

set(AMPLITUDE_COMPILE_DEFINITIONS
    $<IF:$<CONFIG:Shipping>,AMPLITUDE_NO_ASSERTS,>
    $<IF:$<CONFIG:Shipping>,AM_NO_MEMORY_STATS,>
)

# Use these to get the parent path and folder name before adding the external 3rd party target.
get_filename_component(AMPLITUDE_INSTALL_ROOT ${PL_AMPLITUDE_SDK_PATH} DIRECTORY)
get_filename_component(AMPLITUDE_FOLDER ${PL_AMPLITUDE_SDK_PATH} NAME)

if(MSVC)
    set(AMPLITUDE_LIB_OS "win")
    set(AMPLITUDE_LIB_NAME "Amplitude.lib")
    set(AMPLITUDE_LIB_NAME_DEBUG "Amplitude_d.lib")
    set(LIBSAMPLERATE_LIB_NAME "samplerate.lib")
    set(LIBSAMPLERATE_LIB_NAME_DEBUG "samplerate_d.lib")
elseif(APPLE)
    set(AMPLITUDE_LIB_OS "osx")
else()
    set(AMPLITUDE_LIB_OS "linux")
    set(AMPLITUDE_LIB_NAME "libAmplitude.a")
    set(AMPLITUDE_LIB_NAME_DEBUG "libAmplitude_d.a")
    set(LIBSAMPLERATE_LIB_NAME "libsamplerate.a")
    set(LIBSAMPLERATE_LIB_NAME_DEBUG "libsamplerate_d.a")
endif()

add_library(plAmplitude::SDK STATIC IMPORTED GLOBAL)
set_target_properties(plAmplitude::SDK PROPERTIES IMPORTED_LOCATION "${AMPLITUDE_INSTALL_ROOT}/${AMPLITUDE_FOLDER}/sdk/lib/${AMPLITUDE_LIB_OS}/${AMPLITUDE_LIB_NAME}")
set_target_properties(plAmplitude::SDK PROPERTIES IMPORTED_LOCATION_DEBUG "${AMPLITUDE_INSTALL_ROOT}/${AMPLITUDE_FOLDER}/sdk/lib/${AMPLITUDE_LIB_OS}/${AMPLITUDE_LIB_NAME_DEBUG}")
set_target_properties(plAmplitude::SDK PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${AMPLITUDE_INSTALL_ROOT}/${AMPLITUDE_FOLDER}/sdk/include")
pl_uwp_mark_import_as_content(plAmplitude::SDK)

add_library(plAmplitude::libsamplerate STATIC IMPORTED GLOBAL)
set_target_properties(plAmplitude::libsamplerate PROPERTIES IMPORTED_LOCATION "${AMPLITUDE_INSTALL_ROOT}/${AMPLITUDE_FOLDER}/sdk/lib/${AMPLITUDE_LIB_OS}/${LIBSAMPLERATE_LIB_NAME}")
set_target_properties(plAmplitude::libsamplerate PROPERTIES IMPORTED_LOCATION_DEBUG "${AMPLITUDE_INSTALL_ROOT}/${AMPLITUDE_FOLDER}/sdk/lib/${AMPLITUDE_LIB_OS}/${LIBSAMPLERATE_LIB_NAME_DEBUG}")
pl_uwp_mark_import_as_content(plAmplitude::libsamplerate)

######################################
### pl_requires_amplitude()
######################################

macro(pl_requires_amplitude)
    pl_requires(PL_3RDPARTY_AMPLITUDE_SUPPORT)
endmacro()

######################################
### pl_link_target_amplitude(<target>)
######################################

function(pl_link_target_amplitude TARGET_NAME)
    pl_requires_amplitude()

    target_link_libraries(${TARGET_NAME} PRIVATE plAmplitude::SDK)
    target_link_libraries(${TARGET_NAME} PRIVATE plAmplitude::libsamplerate)
endfunction()
