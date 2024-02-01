#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/Blob.h>

#include <Foundation/Memory/AllocatorWithPolicy.h>

plBlob::plBlob() = default;

plBlob::plBlob(plBlob&& other)
{
  m_pStorage = other.m_pStorage;
  m_uiSize = other.m_uiSize;

  other.m_pStorage = nullptr;
  other.m_uiSize = 0;
}

void plBlob::operator=(plBlob&& rhs)
{
  Clear();

  m_pStorage = rhs.m_pStorage;
  m_uiSize = rhs.m_uiSize;

  rhs.m_pStorage = nullptr;
  rhs.m_uiSize = 0;
}

plBlob::~plBlob()
{
  Clear();
}

void plBlob::SetFrom(const void* pSource, plUInt64 uiSize)
{
  SetCountUninitialized(uiSize);
  plMemoryUtils::Copy(static_cast<plUInt8*>(m_pStorage), static_cast<const plUInt8*>(pSource), static_cast<size_t>(uiSize));
}

void plBlob::Clear()
{
  if (m_pStorage)
  {
    plFoundation::GetAlignedAllocator()->Deallocate(m_pStorage);
    m_pStorage = nullptr;
    m_uiSize = 0;
  }
}

void plBlob::SetCountUninitialized(plUInt64 uiCount)
{
  if (m_uiSize != uiCount)
  {
    Clear();

    m_pStorage = plFoundation::GetAlignedAllocator()->Allocate(plMath::SafeConvertToSizeT(uiCount), 64u);
    m_uiSize = uiCount;
  }
}

void plBlob::ZeroFill()
{
  if (m_pStorage)
  {
    plMemoryUtils::ZeroFill(static_cast<plUInt8*>(m_pStorage), static_cast<size_t>(m_uiSize));
  }
}


