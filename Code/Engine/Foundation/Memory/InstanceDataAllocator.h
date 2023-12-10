#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>

/// \brief Structure to describe an instance data type.
///
/// Many resources, such as VMs, state machines and visual scripts of various types have shared state (their configuration)
/// as well as per-instance state (for their execution).
///
/// This structure describes the type of instance data used by a such a resource (or a node inside it).
/// Instance data is allocated through the plInstanceDataAllocator.
///
/// Use the templated Fill() method to fill the desc from a data type.
struct PLASMA_FOUNDATION_DLL plInstanceDataDesc
{
  plUInt32 m_uiTypeSize = 0;
  plUInt32 m_uiTypeAlignment = 0;
  plMemoryUtils::ConstructorFunction m_ConstructorFunction = nullptr;
  plMemoryUtils::DestructorFunction m_DestructorFunction = nullptr;

  template <typename T>
  PLASMA_ALWAYS_INLINE void FillFromType()
  {
    m_uiTypeSize = sizeof(T);
    m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(T);
    m_ConstructorFunction = plMemoryUtils::MakeConstructorFunction<T>();
    m_DestructorFunction = plMemoryUtils::MakeDestructorFunction<T>();
  }
};

/// \brief Helper class to manager instance data allocation, construction and destruction
class PLASMA_FOUNDATION_DLL plInstanceDataAllocator
{
public:
  /// \brief Adds the given desc to internal list of data that needs to be allocated and returns the byte offset.
  [[nodiscard]] plUInt32 AddDesc(const plInstanceDataDesc& desc);

  /// \brief Resets all internal state.
  void ClearDescs();

  /// \brief Constructs the instance data objects, within the pre-allocated memory block.
  void Construct(plByteBlobPtr blobPtr) const;

  /// \brief Destructs the instance data objects.
  void Destruct(plByteBlobPtr blobPtr) const;

  /// \brief Allocates memory and constructs the instance data objects inside it. The returned plBlob must be stored somewhere.
  [[nodiscard]] plBlob AllocateAndConstruct() const;

  /// \brief Destructs and deallocates the instance data objects and the given memory block.
  void DestructAndDeallocate(plBlob& ref_blob) const;

  /// \brief The total size in bytes taken up by all instance data objects that were added.
  plUInt32 GetTotalDataSize() const { return m_uiTotalDataSize; }

  /// \brief Retrieves a void pointer to the instance data within the given blob at the given offset, or nullptr if the offset is invalid.
  PLASMA_ALWAYS_INLINE static void* GetInstanceData(const plByteBlobPtr& blobPtr, plUInt32 uiOffset)
  {
    return (uiOffset != plInvalidIndex) ? blobPtr.GetPtr() + uiOffset : nullptr;
  }

private:
  plDynamicArray<plInstanceDataDesc> m_Descs;
  plUInt32 m_uiTotalDataSize = 0;
};
