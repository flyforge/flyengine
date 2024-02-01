#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Shader/Types.h>
#include <memory>

class plShaderTransform;

class PL_RENDERERCORE_DLL plSkinnedMeshRenderData : public plMeshRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plSkinnedMeshRenderData, plMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;
  plGALBufferHandle m_hSkinningTransforms;
  plArrayPtr<const plUInt8> m_pNewSkinningTransformData;
  std::shared_ptr<bool> m_bTransformsUpdated;
};

struct PL_RENDERERCORE_DLL plSkinningState
{
  plSkinningState();
  ~plSkinningState();

  void Clear();

  /// \brief Holds the current CPU-side copy of the skinning matrices. Modify these and call TransformsChanged() to send them to the GPU.
  plDynamicArray<plShaderTransform, plAlignedAllocatorWrapper> m_Transforms;

  /// \brief Call this, after modifying m_Transforms, to make the renderer apply the update.
  void TransformsChanged();

  void FillSkinnedMeshRenderData(plSkinnedMeshRenderData& ref_renderData) const;

private:
  plGALBufferHandle m_hGpuBuffer;
  std::shared_ptr<bool> m_bTransformsUpdated[2];
};
