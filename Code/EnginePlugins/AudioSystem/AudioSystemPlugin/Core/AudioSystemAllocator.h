#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>

/// \brief Default memory alignment used in the audio system.
#define PLASMA_AUDIOSYSTEM_MEMORY_ALIGNMENT 16

/// \brief An allocator that uses the heap to allocate memory for use by the audio system itself.
/// Audio middleware implementations are not recommended to use this allocator. This allocator will
/// be registered in the plSingletonRegistry at initialization.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioSystemAllocator : public plAlignedHeapAllocator
{
  PLASMA_DECLARE_SINGLETON(plAudioSystemAllocator);

public:
  plAudioSystemAllocator();
};

/// \brief An allocator that uses the heap to allocate memory for use by the audio middleware.
/// Audio middleware implementations should use this allocator to manage memory. This allocator will
/// be registered in the plSingletonRegistry at initialization.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioMiddlewareAllocator : public plAlignedHeapAllocator
{
  PLASMA_DECLARE_SINGLETON(plAudioMiddlewareAllocator);

public:
  /// \brief Constructor.
  /// \param pParentAllocator An instance to the AudioSystemAllocator which will act as the parent of this allocator.
  explicit plAudioMiddlewareAllocator(plAudioSystemAllocator* pParentAllocator);
};

struct plAudioSystemAllocatorWrapper
{
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator()
  {
    return plAudioSystemAllocator::GetSingleton();
  }
};

struct plAudioMiddlewareAllocatorWrapper
{
  PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator()
  {
    return plAudioMiddlewareAllocator::GetSingleton();
  }
};

/// \brief Creates a new instance of type using the audio system allocator.
#define PLASMA_AUDIOSYSTEM_NEW(type, ...) PLASMA_NEW(plAudioSystemAllocatorWrapper::GetAllocator(), type, __VA_ARGS__)

/// \brief Deletes the instance stored in ptr using the audio system allocator and sets ptr to nullptr.
#define PLASMA_AUDIOSYSTEM_DELETE(ptr) PLASMA_DELETE(plAudioSystemAllocatorWrapper::GetAllocator(), ptr)

/// \brief Creates a new array of type using the audio system allocator with count elements, calls default constructor for non-POD types
#define PLASMA_AUDIOSYSTEM_NEW_ARRAY(type, count) PLASMA_NEW_ARRAY(plAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the audio system allocator
#define PLASMA_AUDIOSYSTEM_DELETE_ARRAY(arrayPtr) PLASMA_DELETE_ARRAY(plAudioSystemAllocatorWrapper::GetAllocator(), arrayPtr)

/// \brief Creates a raw buffer of type using the audio system allocator with count elements, but does NOT call the default constructor
#define PLASMA_AUDIOSYSTEM_NEW_RAW_BUFFER(type, count) PLASMA_NEW_RAW_BUFFER(plAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Deletes a raw buffer stored in ptr using the audio system allocator, but does NOT call destructor
#define PLASMA_AUDIOSYSTEM_DELETE_RAW_BUFFER(ptr) PLASMA_DELETE_RAW_BUFFER(plAudioSystemAllocatorWrapper::GetAllocator(), ptr)
