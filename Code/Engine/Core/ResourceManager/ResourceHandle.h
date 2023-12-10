#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief If this is set to PLASMA_ON, stack traces are recorded for every resource handle.
///
/// This can be used to find the places that create resource handles but do not properly clean them up.
#define PLASMA_RESOURCEHANDLE_STACK_TRACES PLASMA_OFF

class plResource;

template <typename T>
class plResourceLock;

// These out-of-line helper functions allow to forward declare resource handles without knowledge about the resource class.
PLASMA_CORE_DLL void IncreaseResourceRefCount(plResource* pResource, const void* pOwner);
PLASMA_CORE_DLL void DecreaseResourceRefCount(plResource* pResource, const void* pOwner);

#if PLASMA_ENABLED(PLASMA_RESOURCEHANDLE_STACK_TRACES)
PLASMA_CORE_DLL void MigrateResourceRefCount(plResource* pResource, const void* pOldOwner, const void* pNewOwner);
#else
PLASMA_ALWAYS_INLINE void MigrateResourceRefCount(plResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
}
#endif

/// \brief The typeless implementation of resource handles. A typed interface is provided by plTypedResourceHandle.
class PLASMA_CORE_DLL plTypelessResourceHandle
{
public:
  PLASMA_ALWAYS_INLINE plTypelessResourceHandle() = default;

  /// \brief [internal] Increases the refcount of the given resource.
  plTypelessResourceHandle(plResource* pResource);

  /// \brief Increases the refcount of the given resource
  PLASMA_ALWAYS_INLINE plTypelessResourceHandle(const plTypelessResourceHandle& rhs)
  {
    m_pResource = rhs.m_pResource;

    if (m_pResource)
    {
      IncreaseResourceRefCount(m_pResource, this);
    }
  }

  /// \brief Move constructor, no refcount change is necessary.
  PLASMA_ALWAYS_INLINE plTypelessResourceHandle(plTypelessResourceHandle&& rhs)
  {
    m_pResource = rhs.m_pResource;
    rhs.m_pResource = nullptr;

    if (m_pResource)
    {
      MigrateResourceRefCount(m_pResource, &rhs, this);
    }
  }

  /// \brief Releases any referenced resource.
  PLASMA_ALWAYS_INLINE ~plTypelessResourceHandle() { Invalidate(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PLASMA_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }

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
  PLASMA_ALWAYS_INLINE bool operator==(const plTypelessResourceHandle& rhs) const { return m_pResource == rhs.m_pResource; }

  /// \brief Checks whether the two handles point to the same resource.
  PLASMA_ALWAYS_INLINE bool operator!=(const plTypelessResourceHandle& rhs) const { return m_pResource != rhs.m_pResource; }

  /// \brief For storing handles as keys in maps
  PLASMA_ALWAYS_INLINE bool operator<(const plTypelessResourceHandle& rhs) const { return m_pResource < rhs.m_pResource; }

  /// \brief Checks whether the handle points to the given resource.
  PLASMA_ALWAYS_INLINE bool operator==(const plResource* rhs) const { return m_pResource == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  PLASMA_ALWAYS_INLINE bool operator!=(const plResource* rhs) const { return m_pResource != rhs; }

protected:
  plResource* m_pResource = nullptr;

private:
  // you must go through the resource manager to get access to the resource pointer
  friend class plResourceManager;
  friend class plResourceHandleWriteContext;
  friend class plResourceHandleReadContext;
  friend class plResourceHandleStreamOperations;
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
  typedef RESOURCE_TYPE ResourceType;

  /// \brief A default constructed handle is invalid and does not reference any resource.
  plTypedResourceHandle() {}

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

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (std::is_base_of<BaseOrDerivedType, ResourceType>::value)
    {
      PLASMA_ASSERT_DEBUG(rhs.IsValid(), "Cannot cast invalid base handle to derived type!");
      plResourceLock<BaseOrDerivedType> lock(rhs, plResourceAcquireMode::PointerOnly);
      PLASMA_ASSERT_DEBUG(plDynamicCast<const ResourceType*>(lock.GetPointer()) != nullptr, "Types are not related!");
    }
#endif
  }

  /// \brief Releases the current reference and increases the refcount of the given resource.
  void operator=(const plTypedResourceHandle<ResourceType>& rhs) { m_hTypeless = rhs.m_hTypeless; }

  /// \brief Move operator, no refcount change is necessary.
  void operator=(plTypedResourceHandle<ResourceType>&& rhs) { m_hTypeless = std::move(rhs.m_hTypeless); }

  /// \brief Checks whether the two handles point to the same resource.
  PLASMA_ALWAYS_INLINE bool operator==(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless == rhs.m_hTypeless; }

  /// \brief Checks whether the two handles point to the same resource.
  PLASMA_ALWAYS_INLINE bool operator!=(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless != rhs.m_hTypeless; }

  /// \brief For storing handles as keys in maps
  PLASMA_ALWAYS_INLINE bool operator<(const plTypedResourceHandle<ResourceType>& rhs) const { return m_hTypeless < rhs.m_hTypeless; }

  /// \brief Checks whether the handle points to the given resource.
  PLASMA_ALWAYS_INLINE bool operator==(const plResource* rhs) const { return m_hTypeless == rhs; }

  /// \brief Checks whether the handle points to the given resource.
  PLASMA_ALWAYS_INLINE bool operator!=(const plResource* rhs) const { return m_hTypeless != rhs; }


  /// \brief Returns the corresponding typeless resource handle.
  PLASMA_ALWAYS_INLINE operator const plTypelessResourceHandle() const { return m_hTypeless; }

  /// \brief Returns the corresponding typeless resource handle.
  PLASMA_ALWAYS_INLINE operator plTypelessResourceHandle() { return m_hTypeless; }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PLASMA_ALWAYS_INLINE bool IsValid() const { return m_hTypeless.IsValid(); }

  /// \brief Returns whether the handle stores a valid pointer to a resource.
  PLASMA_ALWAYS_INLINE explicit operator bool() const { return m_hTypeless.IsValid(); }

  /// \brief Clears any reference to a resource and reduces its refcount.
  PLASMA_ALWAYS_INLINE void Invalidate() { m_hTypeless.Invalidate(); }

  /// \brief Returns the Resource ID hash of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  PLASMA_ALWAYS_INLINE plUInt64 GetResourceIDHash() const { return m_hTypeless.GetResourceIDHash(); }

  /// \brief Returns the Resource ID of the exact resource that this handle points to, without acquiring the resource.
  /// The handle must be valid.
  PLASMA_ALWAYS_INLINE const plString& GetResourceID() const { return m_hTypeless.GetResourceID(); }

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
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plTypedResourceHandle<T>& value) { return plHashingUtils::StringHashTo32(value.GetResourceIDHash()); }

  PLASMA_ALWAYS_INLINE static bool Equal(const plTypedResourceHandle<T>& a, const plTypedResourceHandle<T>& b) { return a == b; }
};

// Stream operations
class plResource;

class PLASMA_CORE_DLL plResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(plStreamWriter& Stream, const plTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(Stream, hResource.m_hTypeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(plStreamReader& Stream, plTypedResourceHandle<ResourceType>& ResourceHandle)
  {
    ReadHandle(Stream, ResourceHandle.m_hTypeless);
  }

private:
  static void WriteHandle(plStreamWriter& Stream, const plResource* pResource);
  static void ReadHandle(plStreamReader& Stream, plTypelessResourceHandle& ResourceHandle);
};

/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator<<(plStreamWriter& Stream, const plTypedResourceHandle<ResourceType>& Value)
{
  plResourceHandleStreamOperations::WriteHandle(Stream, Value);
}

/// \brief Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(plStreamReader& Stream, plTypedResourceHandle<ResourceType>& Value)
{
  plResourceHandleStreamOperations::ReadHandle(Stream, Value);
}
