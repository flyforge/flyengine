#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Gizmos/GizmoBase.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plGizmo::plGizmo()
{
  m_bVisible = false;
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero();
}

void plGizmo::SetVisible(bool bVisible)
{
  if (m_bVisible == bVisible)
    return;

  m_bVisible = bVisible;

  OnVisibleChanged(m_bVisible);
}

void plGizmo::SetTransformation(const plTransform& transform)
{
  if (m_Transformation.IsIdentical(transform))
    return;

  m_Transformation = transform;

  OnTransformationChanged(m_Transformation);
}
