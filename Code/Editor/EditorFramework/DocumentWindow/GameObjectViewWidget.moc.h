#pragma once

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>

class plViewMarqueePickingResultMsgToEditor;
class plQtGameObjectDocumentWindow;
class plOrthoGizmoContext;
class plContextMenuContext;
class plSelectionContext;
class plCameraMoveContext;

class PLASMA_EDITORFRAMEWORK_DLL plQtGameObjectViewWidget : public plQtEngineViewWidget
{
  Q_OBJECT
public:
  plQtGameObjectViewWidget(QWidget* pParent, plQtGameObjectDocumentWindow* pOwnerWindow, plEngineViewConfig* pViewConfig);
  ~plQtGameObjectViewWidget();

  plOrthoGizmoContext* m_pOrthoGizmoContext;
  plSelectionContext* m_pSelectionContext;
  plCameraMoveContext* m_pCameraMoveContext;

  virtual void SyncToEngine() override;

protected:
  virtual void HandleMarqueePickingResult(const plViewMarqueePickingResultMsgToEditor* pMsg) override;

  plUInt32 m_uiLastMarqueeActionID = 0;
  plDeque<plUuid> m_MarqueeBaseSelection;
};

