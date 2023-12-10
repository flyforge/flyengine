#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct plPerInstanceData;
class plInstanceDataProvider;
class plInstancedMeshComponent;

struct PLASMA_RENDERERCORE_DLL plInstanceData
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plInstanceData);

public:
  plInstanceData(plUInt32 uiMaxInstanceCount = 1024);
  ~plInstanceData();

  plGALBufferHandle m_hInstanceDataBuffer;

  plConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(plRenderContext* pRenderContext);

  plArrayPtr<plPerInstanceData> GetInstanceData(plUInt32 uiCount, plUInt32& out_uiOffset);
  void UpdateInstanceData(plRenderContext* pRenderContext, plUInt32 uiCount);

private:
  friend plInstanceDataProvider;
  friend plInstancedMeshComponent;

  void CreateBuffer(plUInt32 uiSize);
  void Reset();

  plUInt32 m_uiBufferSize = 0;
  plUInt32 m_uiBufferOffset = 0;
  plDynamicArray<plPerInstanceData, plAlignedAllocatorWrapper> m_PerInstanceData;
};

class PLASMA_RENDERERCORE_DLL plInstanceDataProvider : public plFrameDataProvider<plInstanceData>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plInstanceDataProvider, plFrameDataProviderBase);

public:
  plInstanceDataProvider();
  ~plInstanceDataProvider();

private:
  virtual void* UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData) override;

  plInstanceData m_Data;
};
