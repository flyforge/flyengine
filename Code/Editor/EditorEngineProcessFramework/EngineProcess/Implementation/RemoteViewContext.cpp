#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plUInt32 plRemoteEngineProcessViewContext::s_uiActiveViewID = 0;
plRemoteEngineProcessViewContext* plRemoteEngineProcessViewContext::s_pActiveRemoteViewContext = nullptr;

plRemoteEngineProcessViewContext::plRemoteEngineProcessViewContext(PlasmaEngineProcessDocumentContext* pContext)
  : PlasmaEngineProcessViewContext(pContext)
{
}

plRemoteEngineProcessViewContext::~plRemoteEngineProcessViewContext()
{
  if (s_pActiveRemoteViewContext == this)
  {
    s_pActiveRemoteViewContext = nullptr;

    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetWorld(nullptr);
    }
  }

  // make sure the base class destructor doesn't destroy the view
  m_hView.Invalidate();
}

void plRemoteEngineProcessViewContext::HandleViewMessage(const PlasmaEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = PlasmaEditorEngineProcessApp::GetSingleton()->CreateRemoteWindowAndView(&m_Camera);
    }

    s_pActiveRemoteViewContext = this;

    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewRedrawMsgToEngine>())
  {
    const plViewRedrawMsgToEngine* pMsg2 = static_cast<const plViewRedrawMsgToEngine*>(pMsg);
    SetCamera(pMsg2);

    // skip the on-message redraw, in remote mode it will just render as fast as it can
    // Redraw(false);
  }
}

plViewHandle plRemoteEngineProcessViewContext::CreateView()
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return plViewHandle();
}
