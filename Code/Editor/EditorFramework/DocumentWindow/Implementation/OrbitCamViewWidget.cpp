#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

plQtOrbitCamViewWidget::plQtOrbitCamViewWidget(plQtEngineDocumentWindow* pOwnerWindow, PlasmaEngineViewConfig* pViewConfig, bool bPicking)
  : plQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = PLASMA_DEFAULT_NEW(plOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  if (bPicking)
  {
    m_pSelectionContext = PLASMA_DEFAULT_NEW(plSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

plQtOrbitCamViewWidget::~plQtOrbitCamViewWidget() = default;


void plQtOrbitCamViewWidget::ConfigureFixed(const plVec3& vCenterPos, const plVec3& vHalfBoxSize, const plVec3& vCamPosition)
{
  m_pOrbitCameraContext->SetDefaultCameraFixed(vCamPosition);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = false;
}

void plQtOrbitCamViewWidget::ConfigureRelative(const plVec3& vCenterPos, const plVec3& vHalfBoxSize, const plVec3& vCamDirection, float fCamDistanceScale)
{
  m_pOrbitCameraContext->SetDefaultCameraRelative(vCamDirection, fCamDistanceScale);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = true;
}

void plQtOrbitCamViewWidget::SetOrbitVolume(const plVec3& vCenterPos, const plVec3& vHalfBoxSize)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);

  if (m_bSetDefaultCamPos)
  {
    if (vHalfBoxSize != plVec3(0.1f))
    {
      // 0.1f is a hard-coded value for the bounding box, in case nothing is available yet
      // not pretty, but somehow we need to know when the first 'proper' bounds are available

      m_bSetDefaultCamPos = false;
      m_pOrbitCameraContext->MoveCameraToDefaultPosition();
    }
  }
}

plOrbitCameraContext* plQtOrbitCamViewWidget::GetOrbitCamera()
{
  return m_pOrbitCameraContext.Borrow();
}

void plQtOrbitCamViewWidget::SyncToEngine()
{
  if (m_pSelectionContext)
  {
    m_pSelectionContext->SetWindowConfig(plVec2I32(width(), height()));
  }

  plQtEngineViewWidget::SyncToEngine();
}
