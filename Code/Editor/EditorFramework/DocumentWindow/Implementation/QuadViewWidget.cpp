#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>

plQtQuadViewWidget::plQtQuadViewWidget(plAssetDocument* pDocument, plQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_ViewFactory = viewFactory;
  m_sViewToolBarMapping = szViewToolBarMapping;

  m_pViewLayout = new QGridLayout(this);
  m_pViewLayout->setContentsMargins(0, 0, 0, 0);
  m_pViewLayout->setSpacing(4);
  setLayout(m_pViewLayout);

  LoadViewConfigs();
}

plQtQuadViewWidget::~plQtQuadViewWidget()
{
  SaveViewConfigs();
}

void plQtQuadViewWidget::SaveViewConfig(const PlasmaEngineViewConfig& cfg, PlasmaEngineViewPreferences& pref) const
{
  pref.m_vCamPos = cfg.m_Camera.GetPosition();
  pref.m_vCamDir = cfg.m_Camera.GetDirForwards();
  pref.m_vCamUp = cfg.m_Camera.GetDirUp();
  pref.m_PerspectiveMode = cfg.m_Perspective;
  pref.m_RenderMode = cfg.m_RenderMode;
  pref.m_fFov = cfg.m_Camera.GetFovOrDim();
}

void plQtQuadViewWidget::LoadViewConfig(PlasmaEngineViewConfig& cfg, PlasmaEngineViewPreferences& pref)
{
  cfg.m_Perspective = (plSceneViewPerspective::Enum)pref.m_PerspectiveMode;
  cfg.m_RenderMode = (plViewRenderMode::Enum)pref.m_RenderMode;
  cfg.m_Camera.LookAt(plVec3(0), plVec3(1, 0, 0), plVec3(0, 0, 1));

  if (cfg.m_Perspective == plSceneViewPerspective::Perspective)
  {
    PlasmaEditorPreferencesUser* pPref = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>();
    cfg.ApplyPerspectiveSetting(pPref->m_fPerspectiveFieldOfView);
  }
  else
  {
    cfg.ApplyPerspectiveSetting(pref.m_fFov);
  }

  pref.m_vCamDir.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();
  pref.m_vCamUp.MakeOrthogonalTo(pref.m_vCamDir);
  pref.m_vCamUp.NormalizeIfNotZero(pref.m_vCamDir.GetOrthogonalVector().GetNormalized()).IgnoreResult();

  cfg.m_Camera.LookAt(pref.m_vCamPos, pref.m_vCamPos + pref.m_vCamDir, pref.m_vCamUp);
}

void plQtQuadViewWidget::SaveViewConfigs() const
{
  plQuadViewPreferencesUser* pPreferences = plPreferences::QueryPreferences<plQuadViewPreferencesUser>(m_pDocument);
  pPreferences->m_bQuadView = m_ActiveMainViews.GetCount() == 4;

  SaveViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  SaveViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  SaveViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  SaveViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  SaveViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);
}

void plQtQuadViewWidget::LoadViewConfigs()
{
  plQuadViewPreferencesUser* pPreferences = plPreferences::QueryPreferences<plQuadViewPreferencesUser>(m_pDocument);

  LoadViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  LoadViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  LoadViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  LoadViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  LoadViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);

  CreateViews(pPreferences->m_bQuadView);
}

void plQtQuadViewWidget::CreateViews(bool bQuad)
{
  plQtScopedUpdatesDisabled _(this);
  for (auto pContainer : m_ActiveMainViews)
  {
    delete pContainer;
  }
  m_ActiveMainViews.Clear();

  if (bQuad)
  {
    for (plUInt32 i = 0; i < 4; ++i)
    {
      plQtEngineViewWidget* pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigQuad[i]);
      plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(m_pWindow, pViewWidget, m_sViewToolBarMapping);
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);
    }
  }
  else
  {
    plQtEngineViewWidget* pViewWidget = m_ViewFactory(m_pWindow, &m_ViewConfigSingle);
    plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(m_pWindow, pViewWidget, m_sViewToolBarMapping);
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);
  }
}

void plQtQuadViewWidget::ToggleViews(QWidget* pView)
{
  plQtEngineViewWidget* pViewport = qobject_cast<plQtEngineViewWidget*>(pView);
  PLASMA_ASSERT_DEV(pViewport != nullptr, "plQtSceneDocumentWindow::ToggleViews must be called with a plQtSceneViewWidget as parameter!");
  bool bIsQuad = m_ActiveMainViews.GetCount() == 4;
  if (bIsQuad)
  {
    m_ViewConfigSingle = *pViewport->m_pViewConfig;
    m_ViewConfigSingle.m_pLinkedViewConfig = pViewport->m_pViewConfig;
    CreateViews(false);
  }
  else
  {
    if (pViewport->m_pViewConfig->m_pLinkedViewConfig != nullptr)
      *pViewport->m_pViewConfig->m_pLinkedViewConfig = *pViewport->m_pViewConfig;

    CreateViews(true);
  }
}
