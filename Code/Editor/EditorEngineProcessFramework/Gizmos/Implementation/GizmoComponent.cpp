#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>

plGizmoComponentManager::plGizmoComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmoRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plGizmoComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plHiddenAttribute(),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plGizmoComponent::plGizmoComponent() = default;
plGizmoComponent::~plGizmoComponent() = default;

plMeshRenderData* plGizmoComponent::CreateRenderData() const
{
  plColor color = m_GizmoColor;

  auto pManager = static_cast<const plGizmoComponentManager*>(GetOwningManager());
  if (GetUniqueID() == pManager->m_uiHighlightID)
  {
    color = plColor(0.9f, 0.9f, 0.1f, color.a);
  }

  plGizmoRenderData* pRenderData = plCreateRenderDataForThisFrame<plGizmoRenderData>(GetOwner());
  pRenderData->m_GizmoColor = color;
  pRenderData->m_bIsPickable = m_bIsPickable;

  return pRenderData;
}

plResult plGizmoComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  plResult r = SUPER::GetLocalBounds(bounds, bAlwaysVisible, msg);

  // adjust the bounds to be mirrored and pretty large, to combat the constant size and face camera modes that are implemented by the shader
  //bounds.m_vBoxHalfExtends = (bounds.m_vCenter + bounds.m_vBoxHalfExtends) * 3.0f;
  //bounds.m_vCenter.SetZero();
  //bounds.m_fSphereRadius = plMath::Max(bounds.m_vBoxHalfExtends.x, bounds.m_vBoxHalfExtends.y, bounds.m_vBoxHalfExtends.z);

  // since there is always only a single gizmo on screen, there's no harm in making it always visible
  bAlwaysVisible = true;
  return r;
}
