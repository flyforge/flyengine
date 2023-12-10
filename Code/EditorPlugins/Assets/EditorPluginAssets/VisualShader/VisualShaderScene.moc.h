#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class plQtNodeView;

class plQtVisualShaderScene : public plQtNodeScene
{
  Q_OBJECT

public:
  plQtVisualShaderScene(QObject* pParent = nullptr);
  ~plQtVisualShaderScene();
};

class plQtVisualShaderPin : public plQtPin
{
public:
  plQtVisualShaderPin();

  virtual void SetPin(const plPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

class plQtVisualShaderNode : public plQtNode
{
public:
  plQtVisualShaderNode();

  virtual void InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

