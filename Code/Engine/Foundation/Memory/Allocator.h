#pragma once

/// \file

#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Id.h>
#include <utility>


#ifdef new
#  undef new
#endif

#ifdef delete
#  undef delete
#endif

using plAllocatorId = plGenericId<24, 8>;

/// \brief Base class for all memory allocators.
class PL_FOUNDATION_DLL plAllocator
{
public:
  struct Stats
  {
    PL_DECLARE_POD_TYPE();

    plUInt64 m_uiNumAllocations = 0;   ///< total number of allocations
    plUInt64 m_uiNumDeallocations = 0; ///< total number of deallocations
    plUInt64 m_uiAllocationSize = 0;   ///< total allocation size in bytes

    plUInt64 m_uiPerFrameAllocationSize = 0; ///< allocation size in bytes in this frame
    plTime m_PerFrameAllocationTime;         ///< time spend on allocations in this frame
  };

  plAllocator();
  virtual ~plAllocator();

  /// \brief Interface, do not use this directly, always use the new/delete macros below
  virtual void* Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc = nullptr) = 0;
  virtual void Deallocate(void* pPtr) = 0;
  virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);

  /// \brief Returns the number of bytes allocated at this address.
  /// 
  /// \note Careful! This information is only available, if allocation tracking is enabled!
  /// Otherwise 0 is returned.
  /// See plAllocatorTrackingMode and PL_ALLOC_TRACKING_DEFAULT.
  virtual size_t AllocatedSize(const void* pPtr) = 0;

  virtual plAllocatorId GetId() const = 0;
  virtual Stats GetStats() const = 0;

private:
  PL_DISALLOW_COPY_AND_ASSIGN(plAllocator);
};

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// \brief creates a new instance of type using the given allocator
#define PL_NEW(allocator, type, ...) \
  plInternal::NewInstance<type>(     \
    new ((allocator)->Allocate(sizeof(type), PL_ALIGNMENT_OF(type), plMemoryUtils::MakeDestructorFunction<type>())) type(__VA_ARGS__), (allocator))

/// \brief deletes the instance stored in ptr using the given allocator and sets ptr to nullptr
#define PL_DELETE(allocator, ptr)       \
  {                                     \
    plInternal::Delete(allocator, ptr); \
    ptr = nullptr;                      \
  }

/// \brief creates a new array of type using the given allocator with count elements, calls default constructor for non-POD types
#define PL_NEW_ARRAY(allocator, type, count) plInternal::CreateArray<type>(allocator, count)

/// \brief Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the given allocator
#define PL_DELETE_ARRAY(allocator, arrayPtr)      \
  {                                               \
    plInternal::DeleteArray(allocator, arrayPtr); \
    arrayPtr.Clear();                             \
  }

/// \brief creates a raw buffer of type using the given allocator with count elements, but does NOT call the default constructor
#define PL_NEW_RAW_BUFFER(allocator, type, count) plInternal::CreateRawBuffer<type>(allocator, count)

/// \brief deletes a raw buffer stored in ptr using the given allocator, but does NOT call destructor
#define PL_DELETE_RAW_BUFFER(allocator, ptr)     \
  {                                              \
    plInternal::DeleteRawBuffer(allocator, ptr); \
    ptr = nullptr;                               \
  }

/// \brief extends a given raw buffer to the new size, taking care of calling constructors / assignment operators.
#define PL_EXTEND_RAW_BUFFER(allocator, ptr, oldSize, newSize) plInternal::ExtendRawBuffer(ptr, allocator, oldSize, newSize)



/// \brief creates a new instance of type using the default allocator
#define PL_DEFAULT_NEW(type, ...) PL_NEW(plFoundation::GetDefaultAllocator(), type, __VA_ARGS__)

/// \brief deletes the instance stored in ptr using the default allocator and sets ptr to nullptr
#define PL_DEFAULT_DELETE(ptr) PL_DELETE(plFoundation::GetDefaultAllocator(), ptr)

/// \brief creates a new array of type using the default allocator with count elements, calls default constructor for non-POD types
#define PL_DEFAULT_NEW_ARRAY(type, count) PL_NEW_ARRAY(plFoundation::GetDefaultAllocator(), type, count)

/// \brief calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the default allocator
#define PL_DEFAULT_DELETE_ARRAY(arrayPtr) PL_DELETE_ARRAY(plFoundation::GetDefaultAllocator(), arrayPtr)

/// \brief creates a raw buffer of type using the default allocator with count elements, but does NOT call the default constructor
#define PL_DEFAULT_NEW_RAW_BUFFER(type, count) PL_NEW_RAW_BUFFER(plFoundation::GetDefaultAllocator(), type, count)

/// \brief deletes a raw buffer stored in ptr using the default allocator, but does NOT call destructor
#define PL_DEFAULT_DELETE_RAW_BUFFER(ptr) PL_DELETE_RAW_BUFFER(plFoundation::GetDefaultAllocator(), ptr)
