#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/InstanceDataAllocator.h>

plUInt32 plInstanceDataAllocator::AddDesc(const plInstanceDataDesc& desc)
{
  m_Descs.PushBack(desc);

  const plUInt32 uiOffset = plMemoryUtils::AlignSize(m_uiTotalDataSize, desc.m_uiTypeAlignment);
  m_uiTotalDataSize = uiOffset + desc.m_uiTypeSize;

  return uiOffset;
}

void plInstanceDataAllocator::ClearDescs()
{
  m_Descs.Clear();
  m_uiTotalDataSize = 0;
}

plBlob plInstanceDataAllocator::AllocateAndConstruct() const
{
  plBlob blob;
  if (m_uiTotalDataSize > 0)
  {
    blob.SetCountUninitialized(m_uiTotalDataSize);
    blob.ZeroFill();

    Construct(blob.GetByteBlobPtr());
  }

  return blob;
}

void plInstanceDataAllocator::DestructAndDeallocate(plBlob& ref_blob) const
{
  PL_ASSERT_DEV(ref_blob.GetByteBlobPtr().GetCount() == m_uiTotalDataSize, "Passed blob has not the expected size");
  Destruct(ref_blob.GetByteBlobPtr());

  ref_blob.Clear();
}

void plInstanceDataAllocator::Construct(plByteBlobPtr blobPtr) const
{
  plUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = plMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_ConstructorFunction != nullptr)
    {
      desc.m_ConstructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}

void plInstanceDataAllocator::Destruct(plByteBlobPtr blobPtr) const
{
  plUInt32 uiOffset = 0;
  for (auto& desc : m_Descs)
  {
    uiOffset = plMemoryUtils::AlignSize(uiOffset, desc.m_uiTypeAlignment);

    if (desc.m_DestructorFunction != nullptr)
    {
      desc.m_DestructorFunction(GetInstanceData(blobPtr, uiOffset));
    }

    uiOffset += desc.m_uiTypeSize;
  }
}


