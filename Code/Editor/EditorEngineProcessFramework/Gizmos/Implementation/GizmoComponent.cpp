#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>

plGizmoComponentManager::plGizmoComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmoRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plGizmoComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plHiddenAttribute(),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
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
  const plResult r = SUPER::GetLocalBounds(bounds, bAlwaysVisible, msg);
  // since there is always only a single gizmo on screen, there's no harm in making it always visible
  bAlwaysVisible = true;
  return r;
}
