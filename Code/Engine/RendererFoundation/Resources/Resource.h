
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALResourceBase : public plRefCounted
{
public:
  void SetDebugName(const char* szName) const
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    m_sDebugName.Assign(szName);
#endif

    SetDebugNamePlatform(szName);
  }

  virtual const plGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class plGALDevice;

  inline ~plGALResourceBase()
  {
    PLASMA_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
    PLASMA_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

    PLASMA_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
    PLASMA_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
    PLASMA_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
  }

  virtual void SetDebugNamePlatform(const char* szName) const = 0;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  mutable plHashedString m_sDebugName;
#endif

  plGALResourceViewHandle m_hDefaultResourceView;
  plGALRenderTargetViewHandle m_hDefaultRenderTargetView;

  plHashTable<plUInt32, plGALResourceViewHandle> m_ResourceViews;
  plHashTable<plUInt32, plGALRenderTargetViewHandle> m_RenderTargetViews;
  plHashTable<plUInt32, plGALUnorderedAccessViewHandle> m_UnorderedAccessViews;
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class plGALResource : public plGALResourceBase
{
public:
  PLASMA_ALWAYS_INLINE plGALResource(const CreationDescription& description)
    : m_Description(description)
  {
  }

  PLASMA_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};
