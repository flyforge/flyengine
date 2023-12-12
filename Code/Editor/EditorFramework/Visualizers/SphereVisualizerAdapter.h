#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plSphereVisualizerAdapter : public plVisualizerAdapter
{
public:
  plSphereVisualizerAdapter();
  ~plSphereVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale;
  plVec3 m_vPositionOffset;
  PlasmaEngineGizmoHandle m_hGizmo;
  plBitflags<plVisualizerAnchor> m_Anchor;
};
