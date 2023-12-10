#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>

plTypelessResourceHandle::plTypelessResourceHandle(plResource* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(m_pResource, this);
  }
}

void plTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
  {
    DecreaseResourceRefCount(m_pResource, this);
  }

  m_pResource = nullptr;
}

plUInt64 plTypelessResourceHandle::GetResourceIDHash() const
{
  return IsValid() ? m_pResource->GetResourceIDHash() : 0;
}

const plString& plTypelessResourceHandle::GetResourceID() const
{
  return m_pResource->GetResourceID();
}

void plTypelessResourceHandle::operator=(const plTypelessResourceHandle& rhs)
{
  PLASMA_ASSERT_DEBUG(this != &rhs, "Cannot assign a resource handle to itself! This would invalidate the handle.");

  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(reinterpret_cast<plResource*>(m_pResource), this);
  }
}

void plTypelessResourceHandle::operator=(plTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource = rhs.m_pResource;
  rhs.m_pResource = nullptr;

  if (m_pResource)
  {
    MigrateResourceRefCount(m_pResource, &rhs, this);
  }
}

// static
void plResourceHandleStreamOperations::WriteHandle(plStreamWriter& Stream, const plResource* pResource)
{
  if (pResource != nullptr)
  {
    Stream << pResource->GetDynamicRTTI()->GetTypeName();
    Stream << pResource->GetResourceID();
  }
  else
  {
    const char* szEmpty = "";
    Stream << szEmpty;
  }
}

// static
void plResourceHandleStreamOperations::ReadHandle(plStreamReader& Stream, plTypelessResourceHandle& ResourceHandle)
{
  plStringBuilder sTemp;

  Stream >> sTemp;
  if (sTemp.IsEmpty())
  {
    ResourceHandle.Invalidate();
    return;
  }

  const plRTTI* pRtti = plRTTI::FindTypeByName(sTemp);
  if (pRtti == nullptr)
  {
    plLog::Error("Unknown resource type '{0}'", sTemp);
    ResourceHandle.Invalidate();
  }

  // read unique ID for restoring the resource (from file)
  Stream >> sTemp;

  if (pRtti != nullptr)
  {
    ResourceHandle = plResourceManager::LoadResourceByType(pRtti, sTemp);
  }
}



PLASMA_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceHandle);
