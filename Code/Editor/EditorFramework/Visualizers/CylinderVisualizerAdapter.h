#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plCylinderVisualizerAdapter : public plVisualizerAdapter
{
public:
  plCylinderVisualizerAdapter();
  ~plCylinderVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius;
  float m_fHeight;
  plVec3 m_vPositionOffset;
  plBitflags<plVisualizerAnchor> m_Anchor;
  plBasisAxis::Enum m_Axis;

  PlasmaEngineGizmoHandle m_hCylinder;
};
