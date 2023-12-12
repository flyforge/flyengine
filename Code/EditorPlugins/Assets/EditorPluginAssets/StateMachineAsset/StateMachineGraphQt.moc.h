#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class plQtStateMachinePin : public plQtPin
{
public:
  plQtStateMachinePin();

  virtual void SetPin(const plPin& pin) override;
  virtual QRectF GetPinRect() const override;
};

class plQtStateMachineConnection : public plQtConnection
{
public:
  plQtStateMachineConnection();
};

class plQtStateMachineNode : public plQtNode
{
public:
  plQtStateMachineNode();

  virtual void InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;
  virtual void ExtendContextMenu(QMenu& menu) override;

  bool IsInitialState() const;
  bool IsAnyState() const;

private:
  void UpdateHeaderColor();
};

class plQtStateMachineAssetScene : public plQtNodeScene
{
  Q_OBJECT

public:
  plQtStateMachineAssetScene(QObject* parent = nullptr);
  ~plQtStateMachineAssetScene();

  void SetInitialState(plQtStateMachineNode* pNode);

private:
  virtual plStatus RemoveNode(plQtNode* pNode) override;
};
