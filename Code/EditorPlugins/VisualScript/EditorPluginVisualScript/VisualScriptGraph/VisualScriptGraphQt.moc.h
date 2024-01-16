#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class plQtVisualScriptPin : public plQtPin
{
public:
  plQtVisualScriptPin();

  virtual void SetPin(const plPin& pin) override;
  virtual bool UpdatePinColors(const plColorGammaUB* pOverwriteColor = nullptr) override;

private:
  void UpdateTooltip();
};

class plQtVisualScriptConnection : public plQtConnection
{
public:
  plQtVisualScriptConnection();
};

class plQtVisualScriptNode : public plQtNode
{
public:
  plQtVisualScriptNode();

  virtual void UpdateState() override;
};

class plQtVisualScriptNodeScene : public plQtNodeScene
{
  Q_OBJECT

public:
  plQtVisualScriptNodeScene(QObject* pParent = nullptr);
  ~plQtVisualScriptNodeScene();

  virtual void InitScene(const plDocumentNodeManager* pManager);

  const QPixmap& GetCoroutineIcon() const { return m_CoroutineIcon; }
  const QPixmap& GetLoopIcon() const { return m_LoopIcon; }

private:
  void GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e);
  void NodeChangedHandler(const plDocumentObject* pObject);

  QPixmap m_CoroutineIcon;
  QPixmap m_LoopIcon;
};
