#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Pipeline/RenderData.h>

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

namespace plRmlUiInternal
{
  struct Vertex
  {
    PLASMA_DECLARE_POD_TYPE();

    plVec3 m_Position;
    plVec2 m_TexCoord;
    plColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    plUInt32 m_uiTriangleCount = 0;
    plGALBufferHandle m_hVertexBuffer;
    plGALBufferHandle m_hIndexBuffer;
    plTexture2DResourceHandle m_hTexture;
  };

  struct Batch
  {
    plMat4 m_Transform = plMat4::MakeIdentity();
    plVec2 m_Translation = plVec2(0);
    CompiledGeometry m_CompiledGeometry;
    plRectFloat m_ScissorRect = plRectFloat(0, 0);
    bool m_bEnableScissorRect = false;
    bool m_bTransformScissorRect = false;
  };
} // namespace plRmlUiInternal

class plRmlUiRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiRenderData, plRenderData);

public:
  plRmlUiRenderData(plAllocatorBase* pAllocator)
    : m_Batches(pAllocator)
  {
  }

  plDynamicArray<plRmlUiInternal::Batch> m_Batches;
};
