#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plMeshComponent, 3, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    PLASMA_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PLASMA_ACCESSOR_PROPERTY("SortingDepthOffset", GetSortingDepthOffset, SetSortingDepthOffset),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plMeshComponent::plMeshComponent() = default;
plMeshComponent::~plMeshComponent() = default;

void plMeshComponent::OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != plWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    plMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    plResourceLock<plMeshResource> pRenderMesh(hRenderMesh, plResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(plResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), plResourceManager::LoadResource<plCpuMeshResource>(GetMeshFile()));
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
