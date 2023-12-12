#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plBoxVisualizerAdapter : public plVisualizerAdapter
{
public:
  plBoxVisualizerAdapter();
  ~plBoxVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  plVec3 m_vScale;
  plVec3 m_vPositionOffset;
  plQuat m_qRotation;
  plBitflags<plVisualizerAnchor> m_Anchor;
  PlasmaEngineGizmoHandle m_hGizmo;
};
