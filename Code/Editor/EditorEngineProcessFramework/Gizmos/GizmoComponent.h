#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoRenderData : public plMeshRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plGizmoRenderData, plMeshRenderData);

public:
  plColor m_GizmoColor;
  bool m_bIsPickable;
};

class plGizmoComponent;
class plGizmoComponentManager : public plComponentManager<plGizmoComponent, plBlockStorageType::FreeList>
{
public:
  plGizmoComponentManager(plWorld* pWorld);

  plUInt32 m_uiHighlightID = 0;
};

/// \brief Used by the editor to render gizmo meshes.
///
/// Gizmos use special shaders to have constant screen-space size and swap geometry towards the viewer,
/// so their culling is non-trivial. This component takes care of that and of the highlight color.
class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoComponent : public plMeshComponent
{
  PL_DECLARE_COMPONENT_TYPE(plGizmoComponent, plMeshComponent, plGizmoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plMeshComponentBase

protected:
  virtual plMeshRenderData* CreateRenderData() const override;
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plGizmoComponent

public:
  plGizmoComponent();
  ~plGizmoComponent();

  plColor m_GizmoColor = plColor::White;
  bool m_bIsPickable = true;
};
