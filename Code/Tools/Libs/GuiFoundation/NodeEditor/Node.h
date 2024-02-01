#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsWidget>

// Avoid conflicts with windows.
#ifdef GetObject
#  undef GetObject
#endif

class plQtPin;
class plDocumentNodeManager;
class QLabel;
class plDocumentObject;
class QGraphicsTextItem;
class QGraphicsPixmapItem;
class QGraphicsDropShadowEffect;

struct plNodeFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    Moved = PL_BIT(0),
    UpdateTitle = PL_BIT(1),
    Default = None
  };

  struct Bits
  {
    StorageType Moved : 1;
    StorageType UpdateTitle : 1;
  };
};

class PL_GUIFOUNDATION_DLL plQtNode : public QGraphicsPathItem
{
public:
  plQtNode();
  ~plQtNode();
  virtual int type() const override { return plQtNodeScene::Node; }

  const plDocumentObject* GetObject() const { return m_pObject; }
  virtual void InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject);

  virtual void UpdateGeometry();

  void CreatePins();

  plQtPin* GetInputPin(const plPin& pin);
  plQtPin* GetOutputPin(const plPin& pin);

  plBitflags<plNodeFlags> GetFlags() const;
  void ResetFlags();

  void EnableDropShadow(bool bEnable);
  virtual void UpdateState();

  const plHybridArray<plQtPin*, 6>& GetInputPins() const { return m_Inputs; }
  const plHybridArray<plQtPin*, 6>& GetOutputPins() const { return m_Outputs; }

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  QColor m_HeaderColor;
  QRectF m_HeaderRect;
  QGraphicsTextItem* m_pTitleLabel = nullptr;
  QGraphicsTextItem* m_pSubtitleLabel = nullptr;
  QGraphicsPixmapItem* m_pIcon = nullptr;

private:
  const plDocumentNodeManager* m_pManager = nullptr;
  const plDocumentObject* m_pObject = nullptr;
  plBitflags<plNodeFlags> m_DirtyFlags;

  bool m_bIsActive = true;

  QGraphicsDropShadowEffect* m_pShadow = nullptr;

  // Pins
  plHybridArray<plQtPin*, 6> m_Inputs;
  plHybridArray<plQtPin*, 6> m_Outputs;
};
