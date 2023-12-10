#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class plQtProcGenNode : public plQtNode
{
public:
  plQtProcGenNode();

  virtual void InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

class plQtProcGenPin : public plQtPin
{
public:
  plQtProcGenPin();
  ~plQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& menu) override;

  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

class plQtProcGenScene : public plQtNodeScene
{
public:
  plQtProcGenScene(QObject* parent = nullptr);
  ~plQtProcGenScene();

  void SetDebugPin(plQtProcGenPin* pDebugPin);

private:
  virtual plStatus RemoveNode(plQtNode* pNode) override;

  bool m_bUpdatingDebugPin = false;
  plQtProcGenPin* m_pDebugPin = nullptr;
};
