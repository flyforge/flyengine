#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plCapsuleVisualizerAdapter : public plVisualizerAdapter
{
public:
  plCapsuleVisualizerAdapter();
  ~plCapsuleVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fRadius = 0.0f;
  float m_fHeight = 0.0f;
  plBitflags<plVisualizerAnchor> m_Anchor;

  PlasmaEngineGizmoHandle m_hSphereTop;
  PlasmaEngineGizmoHandle m_hSphereBottom;
  PlasmaEngineGizmoHandle m_hCylinder;
};
