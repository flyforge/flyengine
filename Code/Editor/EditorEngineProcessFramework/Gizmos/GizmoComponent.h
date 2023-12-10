#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoRenderData : public plMeshRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGizmoRenderData, plMeshRenderData);

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

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoComponent : public plMeshComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plGizmoComponent, plMeshComponent, plGizmoComponentManager);

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

  plColor m_GizmoColor;
  bool m_bIsPickable = true;
};
