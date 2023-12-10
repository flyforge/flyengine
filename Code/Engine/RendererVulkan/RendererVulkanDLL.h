#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define PLASMA_RENDERERVULKAN_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_RENDERERVULKAN_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
//#define VK_LOG_LAYOUT_CHANGES

#define PLASMA_GAL_VULKAN_RELEASE(vulkanObj) \
  do                                     \
  {                                      \
    if ((vulkanObj) != nullptr)          \
    {                                    \
      (vulkanObj)->Release();            \
      (vulkanObj) = nullptr;             \
    }                                    \
  } while (0)

#define VK_ASSERT_DEBUG(code)                                                                                           \
  do                                                                                                                    \
  {                                                                                                                     \
    auto s = (code);                                                                                                    \
    PLASMA_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      PLASMA_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE);            \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                           \
  do                                                                                                                  \
  {                                                                                                                   \
    auto s = (code);                                                                                                  \
    PLASMA_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      PLASMA_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE);          \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                    \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
      return s;                                                                                                                                                           \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_PLASMA_FAILURE(code)                                                                                                                             \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PLASMA_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE); \
      return PLASMA_FAILURE;                                                                                                                                                  \
    }                                                                                                                                                                     \
  } while (false)
