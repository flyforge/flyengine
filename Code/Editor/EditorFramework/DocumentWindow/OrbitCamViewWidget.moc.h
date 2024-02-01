#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Basics.h>
#include <Foundation/Types/UniquePtr.h>

class plOrbitCameraContext;
class plSelectionContext;

class PL_EDITORFRAMEWORK_DLL plQtOrbitCamViewWidget : public plQtEngineViewWidget
{
  Q_OBJECT
public:
  plQtOrbitCamViewWidget(plQtEngineDocumentWindow* pOwnerWindow, plEngineViewConfig* pViewConfig, bool bPicking = false);
  ~plQtOrbitCamViewWidget();

  void ConfigureFixed(const plVec3& vCenterPos, const plVec3& vHalfBoxSize, const plVec3& vCamPosition);
  void ConfigureRelative(const plVec3& vCenterPos, const plVec3& vHalfBoxSize, const plVec3& vCamDirection, float fCamDistanceScale);

  void SetOrbitVolume(const plVec3& vCenterPos, const plVec3& vHalfBoxSize);

  plOrbitCameraContext* GetOrbitCamera();

  virtual void SyncToEngine() override;

private:
  bool m_bSetDefaultCamPos = true;

  plUniquePtr<plOrbitCameraContext> m_pOrbitCameraContext;
  plUniquePtr<plSelectionContext> m_pSelectionContext;
};
