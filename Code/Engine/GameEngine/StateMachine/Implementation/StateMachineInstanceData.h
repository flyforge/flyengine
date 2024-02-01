#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

namespace plStateMachineInternal
{
  /// \brief Helper class to manage instance data for compound states or transitions
  struct PL_GAMEENGINE_DLL Compound
  {
    PL_ALWAYS_INLINE plUInt32 GetBaseOffset() const { return m_InstanceDataOffsets.GetUserData<plUInt32>(); }
    PL_ALWAYS_INLINE plUInt32 GetDataSize() const { return m_InstanceDataAllocator.GetTotalDataSize(); }

    plSmallArray<plUInt32, 2> m_InstanceDataOffsets;
    plInstanceDataAllocator m_InstanceDataAllocator;

    struct InstanceData
    {
      const Compound* m_pOwner = nullptr;

      ~InstanceData()
      {
        if (m_pOwner != nullptr)
        {
          m_pOwner->m_InstanceDataAllocator.Destruct(GetBlobPtr());
        }
      }

      PL_ALWAYS_INLINE plByteBlobPtr GetBlobPtr()
      {
        return plByteBlobPtr(plMemoryUtils::AddByteOffset(reinterpret_cast<plUInt8*>(this), m_pOwner->GetBaseOffset()), m_pOwner->GetDataSize());
      }
    };

    PL_ALWAYS_INLINE void* GetSubInstanceData(InstanceData* pData, plUInt32 uiIndex) const
    {
      return pData != nullptr ? m_InstanceDataAllocator.GetInstanceData(pData->GetBlobPtr(), m_InstanceDataOffsets[uiIndex]) : nullptr;
    }

    PL_FORCE_INLINE void Initialize(InstanceData* pData) const
    {
      if (pData != nullptr && pData->m_pOwner == nullptr)
      {
        pData->m_pOwner = this;
        m_InstanceDataAllocator.Construct(pData->GetBlobPtr());
      }
    }

    template <typename T>
    bool GetInstanceDataDesc(plArrayPtr<T*> subObjects, plInstanceDataDesc& out_desc)
    {
      m_InstanceDataOffsets.Clear();
      m_InstanceDataAllocator.ClearDescs();

      plUInt32 uiMaxAlignment = 0;

      plInstanceDataDesc instanceDataDesc;
      for (T* pSubObject : subObjects)
      {
        plUInt32 uiOffset = plInvalidIndex;
        if (pSubObject->GetInstanceDataDesc(instanceDataDesc))
        {
          uiOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
          uiMaxAlignment = plMath::Max(uiMaxAlignment, instanceDataDesc.m_uiTypeAlignment);
        }
        m_InstanceDataOffsets.PushBack(uiOffset);
      }

      if (uiMaxAlignment > 0)
      {
        out_desc.FillFromType<InstanceData>();
        out_desc.m_ConstructorFunction = nullptr; // not needed, instance data is constructed on first OnEnter

        plUInt32 uiBaseOffset = plMemoryUtils::AlignSize(out_desc.m_uiTypeSize, uiMaxAlignment);
        m_InstanceDataOffsets.GetUserData<plUInt32>() = uiBaseOffset;

        out_desc.m_uiTypeSize = uiBaseOffset + m_InstanceDataAllocator.GetTotalDataSize();
        out_desc.m_uiTypeAlignment = plMath::Max(out_desc.m_uiTypeAlignment, uiMaxAlignment);

        return true;
      }

      return false;
    }
  };
} // namespace plStateMachineInternal
