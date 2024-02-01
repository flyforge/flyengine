#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of meshes with procedural generated vertex colors
class PL_PROCGENPLUGIN_DLL plProcVertexColorRenderer : public plMeshRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plProcVertexColorRenderer, plMeshRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plProcVertexColorRenderer);

public:
  plProcVertexColorRenderer();
  ~plProcVertexColorRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;

protected:
  virtual void SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const override;
  virtual void FillPerInstanceData(
    plArrayPtr<plPerInstanceData> instanceData, const plRenderDataBatch& batch, plUInt32 uiStartIndex, plUInt32& out_uiFilteredCount) const override;
};
