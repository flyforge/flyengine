#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plCamera;

struct plGizmoEvent
{
  enum class Type
  {
    BeginInteractions,
    EndInteractions,
    Interaction,
    CancelInteractions,
  };

  const plEditorInputContext* m_pGizmo = nullptr;
  Type m_Type;
};

class PLASMA_EDITORFRAMEWORK_DLL plGizmo : public plEditorInputContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGizmo, plEditorInputContext);

public:
  plGizmo();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const plTransform& transform);
  const plTransform& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(plGizmoHandle* pHandle, const plCamera* pCamera, const plVec3& vInteractionPivot, const plVec2I32& vViewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera = pCamera;
    m_vInteractionPivot = vInteractionPivot;
    m_vViewport = vViewport;
  }

  plEvent<const plGizmoEvent&> m_GizmoEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible) = 0;
  virtual void OnTransformationChanged(const plTransform& transform) = 0;

  const plCamera* m_pCamera;
  plGizmoHandle* m_pInteractionGizmoHandle;
  plVec3 m_vInteractionPivot;
  plVec2I32 m_vViewport;

private:
  bool m_bVisible;
  plTransform m_Transformation;
};
