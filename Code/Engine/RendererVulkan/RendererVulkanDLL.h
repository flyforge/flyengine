#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERVULKAN_LIB
#    define PL_RENDERERVULKAN_DLL PL_DECL_EXPORT
#  else
#    define PL_RENDERERVULKAN_DLL PL_DECL_IMPORT
#  endif
#else
#  define PL_RENDERERVULKAN_DLL
#endif

// Uncomment to log all layout transitions.
//#define VK_LOG_LAYOUT_CHANGES

#define PL_GAL_VULKAN_RELEASE(vulkanObj) \
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
    PL_ASSERT_DEBUG(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      PL_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PL_SOURCE_FILE, PL_SOURCE_LINE);            \
  } while (false)

#define VK_ASSERT_DEV(code)                                                                                           \
  do                                                                                                                  \
  {                                                                                                                   \
    auto s = (code);                                                                                                  \
    PL_ASSERT_DEV(static_cast<vk::Result>(s) == vk::Result::eSuccess, "Vukan call '{0}' failed with: {1} in {2}:{3}", \
      PL_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PL_SOURCE_FILE, PL_SOURCE_LINE);          \
  } while (false)

#define VK_LOG_ERROR(code)                                                                                                                                                \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PL_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PL_SOURCE_FILE, PL_SOURCE_LINE); \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_LOG(code)                                                                                                                                    \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PL_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PL_SOURCE_FILE, PL_SOURCE_LINE); \
      return s;                                                                                                                                                           \
    }                                                                                                                                                                     \
  } while (false)

#define VK_SUCCEED_OR_RETURN_PL_FAILURE(code)                                                                                                                             \
  do                                                                                                                                                                      \
  {                                                                                                                                                                       \
    auto s = (code);                                                                                                                                                      \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                               \
    {                                                                                                                                                                     \
      plLog::Error("Vukan call '{0}' failed with: {1} in {2}:{3}", PL_STRINGIZE(code), vk::to_string(static_cast<vk::Result>(s)).data(), PL_SOURCE_FILE, PL_SOURCE_LINE); \
      return PL_FAILURE;                                                                                                                                                  \
    }                                                                                                                                                                     \
  } while (false)
