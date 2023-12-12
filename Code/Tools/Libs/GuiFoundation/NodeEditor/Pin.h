#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsPathItem>

class plPin;
class plQtConnection;

enum class plQtPinHighlightState
{
  None,
  CannotConnect,
  CannotConnectSameDirection,
  CanAddConnection,
  CanReplaceConnection,
};

class PLASMA_GUIFOUNDATION_DLL plQtPin : public QGraphicsPathItem
{
public:
  plQtPin();
  ~plQtPin();
  virtual int type() const override { return plQtNodeScene::Pin; }

  void AddConnection(plQtConnection* pConnection);
  void RemoveConnection(plQtConnection* pConnection);
  plArrayPtr<plQtConnection*> GetConnections() { return m_Connections; }
  bool HasAnyConnections() const { return !m_Connections.IsEmpty(); }

  const plPin* GetPin() const { return m_pPin; }
  virtual void SetPin(const plPin& pin);
  virtual void ConnectedStateChanged(bool bConnected);

  virtual QPointF GetPinPos() const;
  virtual QPointF GetPinDir() const;
  virtual QRectF GetPinRect() const;
  virtual void UpdateConnections();
  void SetHighlightState(plQtPinHighlightState state);

  void SetActive(bool active);

  virtual void ExtendContextMenu(QMenu& menu) {}
  virtual void keyPressEvent(QKeyEvent* event) override {}

protected:
  virtual bool UpdatePinColors(const plColorGammaUB* pOverwriteColor = nullptr);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  plQtPinHighlightState m_HighlightState = plQtPinHighlightState::None;
  QGraphicsTextItem* m_pLabel;
  QPointF m_PinCenter;

  bool m_bTranslatePinName = true;
private:
  bool m_bIsActive = true;

  const plPin* m_pPin = nullptr;
  plHybridArray<plQtConnection*, 6> m_Connections;
};
