#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectEditTool, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plGameObjectEditTool::plGameObjectEditTool() {}

void plGameObjectEditTool::ConfigureTool(
  plGameObjectDocument* pDocument, plQtGameObjectDocumentWindow* pWindow, plGameObjectGizmoInterface* pInterface)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_pInterface = pInterface;

  OnConfigured();
}

void plGameObjectEditTool::SetActive(bool active)
{
  if (m_bIsActive == active)
    return;

  m_bIsActive = active;
  OnActiveChanged(m_bIsActive);

  if (!m_bIsActive)
  {
    m_pWindow->SetPermanentStatusBarMsg("");
  }
}
