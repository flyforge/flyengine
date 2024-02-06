#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>

/// \brief Default memory alignment used in the audio system.
#define PL_AUDIOSYSTEM_MEMORY_ALIGNMENT 16

/// \brief An allocator that uses the heap to allocate memory for use by the audio system itself.
/// Audio middleware implementations are not recommended to use this allocator. This allocator will
/// be registered in the plSingletonRegistry at initialization.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioSystemAllocator : public plAlignedHeapAllocator
{
  PL_DECLARE_SINGLETON(plAudioSystemAllocator);

public:
  plAudioSystemAllocator();
};

/// \brief An allocator that uses the heap to allocate memory for use by the audio middleware.
/// Audio middleware implementations should use this allocator to manage memory. This allocator will
/// be registered in the plSingletonRegistry at initialization.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioMiddlewareAllocator : public plAlignedHeapAllocator
{
  PL_DECLARE_SINGLETON(plAudioMiddlewareAllocator);

public:
  /// \brief Constructor.
  /// \param pParentAllocator An instance to the AudioSystemAllocator which will act as the parent of this allocator.
  explicit plAudioMiddlewareAllocator(plAudioSystemAllocator* pParentAllocator);
};

struct plAudioSystemAllocatorWrapper
{
  PL_ALWAYS_INLINE static plAllocator* GetAllocator()
  {
    return plAudioSystemAllocator::GetSingleton();
  }
};

struct plAudioMiddlewareAllocatorWrapper
{
  PL_ALWAYS_INLINE static plAllocator* GetAllocator()
  {
    return plAudioMiddlewareAllocator::GetSingleton();
  }
};

/// \brief Creates a new instance of type using the audio system allocator.
#define PL_AUDIOSYSTEM_NEW(type, ...) PL_NEW(plAudioSystemAllocatorWrapper::GetAllocator(), type, __VA_ARGS__)

/// \brief Deletes the instance stored in ptr using the audio system allocator and sets ptr to nullptr.
#define PL_AUDIOSYSTEM_DELETE(ptr) PL_DELETE(plAudioSystemAllocatorWrapper::GetAllocator(), ptr)

/// \brief Creates a new array of type using the audio system allocator with count elements, calls default constructor for non-POD types
#define PL_AUDIOSYSTEM_NEW_ARRAY(type, count) PL_NEW_ARRAY(plAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the audio system allocator
#define PL_AUDIOSYSTEM_DELETE_ARRAY(arrayPtr) PL_DELETE_ARRAY(plAudioSystemAllocatorWrapper::GetAllocator(), arrayPtr)

/// \brief Creates a raw buffer of type using the audio system allocator with count elements, but does NOT call the default constructor
#define PL_AUDIOSYSTEM_NEW_RAW_BUFFER(type, count) PL_NEW_RAW_BUFFER(plAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Deletes a raw buffer stored in ptr using the audio system allocator, but does NOT call destructor
#define PL_AUDIOSYSTEM_DELETE_RAW_BUFFER(ptr) PL_DELETE_RAW_BUFFER(plAudioSystemAllocatorWrapper::GetAllocator(), ptr)
