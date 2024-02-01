#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief If this is set to PL_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define PL_RESOURCEHANDLE_STACK_TRACES PL_OFF

class plResource;

template <typename T>
class plResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
PL_CORE_DLL void IncreaseResourceRefCount(plResource* pResource, const void* pOwner);
PL_CORE_DLL void DecreaseResourceRefCount(plResource* pResource, const void* pOwner);

#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)
PL_CORE_DLL void MigrateResourceRefCount(plResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
PL_ALWAYS_INLINE void MigrateResourceRefCount(plResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
}
#endif

/// \brief The typeless implementation of resource handles. A typed interface is provided by plTypedResourceHandle.
class PL_CORE_DLL plTypelessResourceHandle
{
public:
  PL_ALWAYS_INLINE plTypelessResourceHandle() = default;

  /// \brief [internal] Increases the refcount of the given resource.
  plTypelessResourceHandle(plResource* pResource);

  /// \brief Increases the refcount of the given resource
  PL_ALWAYS_INLINE plTypelessResourceHandle(const plTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// \brief Move constructor, no refcount change is necessary.
  PL_ALWAYS_INLINE plTypelessResourceHandle(plTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// \brief Releases any referenced resource.
  PL_ALWAYS_INLINE ~plTypelessResourceHandle() { Invalidate(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PL_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

  /// \brief Clears any reference to a resource and reduces its refcount.
  void Invalidate();

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  plUInt64 GetResourceIDHash() const;

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  const plString& GetResourceID() const;

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const plTypelessResourceHandle& rhs);

  /// \brief Move operator, no refcount change is necessary.
  void operator=(plTypelessResourceHandle&& rhs);

  /// \brief Checks whether the two handles point to the same resource.
  PL_ALWAYS_INLINE bool operator==(const plTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// \brief Checks whether the two handles point to the same resource.
  PL_ALWAYS_INLINE bool operator!=(const plTypelessResourceHandle& rhs) const { return m_pResource != rhs.m_pResource; }

  /// \brief For storing handles as keys in maps
  PL_ALWAYS_INLINE bool operator<(const plTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// \brief Checks whether the handle points to the given resource.
  PL_ALWAYS_INLINE bool operator==(const plResource* rhs) const { return m_pResource == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  PL_ALWAYS_INLINE bool operator!=(const plResource* rhs) const { return m_pResource != rhs; }

  /// \brief Returns the type information of the resource or nullptr if the handle is invalid.
  const plRTTI* GetResourceType() const;

protected:
  plResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class plResourceManager;
  friend class plResourceHandleWriteContext;
  friend class plResourceHandleReadContext;
  friend class plResourceHandleStreamOperations;
};

template <>
struct plHashHelper<plTypelessResourceHandle>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plTypelessResourceHandle& value) { return plHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  PL_ALWAYS_INLINE static bool Equal(const plTypelessResourceHandle& a, const plTypelessResourceHandle& b) { return a == b; }
};

/// \brief The plTypedResourceHandle controls access to an plResource.
///
/// All resources must be referenced using plTypedResourceHandle instances (instantiated with the proper resource type as the template
/// argument). You must not store a direct pointer to a resource anywhere. Instead always store resource handles. To actually access a
/// resource, use plResourceManager::BeginAcquireResource and plResourceManager::EndAcquireResource after you have finished using it.
///
/// plTypedResourceHandle implements reference counting on resources. It also allows to redirect resources to fallback resources when they
/// are not yet loaded (if possible).
///
/// As long as there is one resource handle that references a resource, it is considered 'in use' and thus might not get unloaded.
/// So be careful where you store resource handles.
/// If necessary you can call Invalidate() to clear a resource handle and thus also remove the reference to the resource.
template <typename RESOURCE_TYPE>
class plTypedResourceHandle
{
public:
  using ResourceType = RESOURCE_TYPE;

  /// \brief A default constructed handle is invalid and does not reference any resource.
  plTypedResourceHandle() = default;

  /// \brief Increases the refcount of the given resource.
  explicit plTypedResourceHandle(ResourceType* pResource)
    : m_hTypeless(pResource)
  {
  }

  /// \brief Increases the refcount of the given resource.
  plTypedResourceHandle(const plTypedResourceHandle<ResourceType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
  }

  /// \brief Move constructor, no refcount change is necessary.
  plTypedResourceHandle(plTypedResourceHandle<ResourceType>&& rhs)
    : m_hTypeless(std::move(rhs.m_hTypeless))
  {
  }

  template <typename BaseOrDerivedType>
  plTypedResourceHandle(const plTypedResourceHandle<BaseOrDerivedType>& rhs)
    : m_hTypeless(rhs.m_hTypeless)
  {
    static_assert(std::is_base_of<ResourceType, BaseOrDerivedType>::value || std::is_base_of<BaseOrDerivedType, ResourceType>::value, "Only related types can be assigned to handles of this type");

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      PL_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      plResourceLock<BaseOrDerivedType> lock(rhs, plResourceAcquireMode::PointerOnly);
      PL_ASSERT_DEBUG(plDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const plTypedResourceHandle<ResourceType>& rhs) { m_hTypeless = rhs.m_hTypeless; }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(plTypedResourceHandle<ResourceType>&& rhs) { m_hTypeless = std::move(rhs.m_hTypeless); }

  /// \brief Checks whether the two handles point to the same resource.
  PL_ALWAYS_INLINE bool operator==(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless == rhs.m_hTypeless; }

  /// \brief Checks whether the two handles point to the same resource.
  PL_ALWAYS_INLINE bool operator!=(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless != rhs.m_hTypeless; }

  /// \brief For storing handles as keys in maps
  PL_ALWAYS_INLINE bool operator<(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless < rhs.m_hTypeless; }

  /// \brief Checks whether the handle points to the given resource.
  PL_ALWAYS_INLINE bool operator==(const plResource* rhs) const { return m_hTypeless == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  PL_ALWAYS_INLINE bool operator!=(const plResource* rhs) const { return m_hTypeless != rhs; }


  /// \brief Returns the corresponding typeless resource handle.
  PL_ALWAYS_INLINE operator const plTypelessResourceHandle() const { return m_hTypeless; }

  /// \brief Returns the corresponding typeless resource handle.
  PL_ALWAYS_INLINE operator plTypelessResourceHandle() { return m_hTypeless; }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PL_ALWAYS_INLINE bool IsValid() const { return m_hTypeless.IsValid(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PL_ALWAYS_INLINE explicit operator bool() const { return m_hTypeless.IsValid(); }

  /// \brief Clears any reference to a resource and reduces its refcount.
  PL_ALWAYS_INLINE void Invalidate() { m_hTypeless.Invalidate(); }

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  PL_ALWAYS_INLINE plUInt64 GetResourceIDHash() const { return m_hTypeless.GetResourceIDHash(); }

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  PL_ALWAYS_INLINE const plString& GetResourceID() const { return m_hTypeless.GetResourceID(); }

  /// \brief Attempts to copy the given typeless handle to this handle.
  ///
  /// It is an error to assign a typeless handle that references a resource with a mismatching type.
  void AssignFromTypelessHandle(const plTypelessResourceHandle& hHandle)
  {
    if (!hHandle.IsValid())
      return;

    PL_ASSERT_DEV(hHandle.GetResourceType()->IsDerivedFrom<RESOURCE_TYPE>(), "Type '{}' does not match resource type '{}' in typeless handle.", plGetStaticRTTI<RESOURCE_TYPE>()->GetTypeName(), hHandle.GetResourceType()->GetTypeName());

    m_hTypeless = hHandle;
  }

private:
  template <typename T>
  friend class plTypedResourceHandle;

  // you must go through the resource manager to get access to the resource pointer
  friend class plResourceManager;
  friend class plResourceHandleWriteContext;
  friend class plResourceHandleReadContext;
  friend class plResourceHandleStreamOperations;

  plTypelessResourceHandle m_hTypeless;
};

template <typename T>
struct plHashHelper<plTypedResourceHandle<T>>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plTypedResourceHandle<T>& value) { return plHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  PL_ALWAYS_INLINE static bool Equal(const plTypedResourceHandle<T>& a, const plTypedResourceHandle<T>& b) { return a == b; }
};

// Stream operations
class plResource;

class PL_CORE_DLL plResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(plStreamWriter& inout_stream, const plTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(inout_stream, hResource.m_hTypeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(plStreamReader& inout_stream, plTypedResourceHandle<ResourceType>& ref_hResourceHandle)
  {
    ReadHandle(inout_stream, ref_hResourceHandle.m_hTypeless);
  }

private:
  static void WriteHandle(plStreamWriter& Stream, const plResource* pResource);
  static void ReadHandle(plStreamReader& Stream, plTypelessResourceHandle& ResourceHandle);
};

/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator<<(plStreamWriter& inout_stream, const plTypedResourceHandle<ResourceType>& hValue)
{
  plResourceHandleStreamOperations::WriteHandle(inout_stream, hValue);
}

/// \brief Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(plStreamReader& inout_stream, plTypedResourceHandle<ResourceType>& ref_hValue)
{
  plResourceHandleStreamOperations::ReadHandle(inout_stream, ref_hValue);
}
