#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <ToolsFoundation/Document/Document.h>

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QPainter>

plQtNode::plQtNode()
{
  auto palette = QApplication::palette();

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setBrush(palette.window());
  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);

  {
    QFont font = QApplication::font();
    font.setBold(true);

    m_pTitleLabel = new QGraphicsTextItem(this);
    m_pTitleLabel->setFont(font);
  }

  {
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.9f);

    m_pSubtitleLabel = new QGraphicsTextItem(this);
    m_pSubtitleLabel->setFont(font);
    m_pSubtitleLabel->setPos(0, m_pTitleLabel->boundingRect().bottom() - 5);
  }

  {
    m_pIcon = new QGraphicsPixmapItem(this);
  }

  m_HeaderColor = palette.highlight().color();
}

plQtNode::~plQtNode()
{
  EnableDropShadow(false);
}

void plQtNode::EnableDropShadow(bool enable)
{
  if (enable && m_pShadow == nullptr)
  {
    auto palette = QApplication::palette();

    m_pShadow = new QGraphicsDropShadowEffect();
    m_pShadow->setOffset(3, 3);
    m_pShadow->setColor(palette.color(QPalette::Shadow));
    m_pShadow->setBlurRadius(10);
    setGraphicsEffect(m_pShadow);
  }

  if (!enable && m_pShadow != nullptr)
  {
    delete m_pShadow;
    m_pShadow = nullptr;
  }
}

void plQtNode::InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject)
{
  m_pManager = pManager;
  m_pObject = pObject;
  CreatePins();
  UpdateState();

  UpdateGeometry();

  if (const plColorAttribute* pColorAttr = pObject->GetType()->GetAttributeByType<plColorAttribute>())
  {
    m_HeaderColor = plToQtColor(pColorAttr->GetColor());
  }

  m_DirtyFlags.Add(plNodeFlags::UpdateTitle);
}

void plQtNode::UpdateGeometry()
{
  prepareGeometryChange();

  QRectF iconRect = m_pIcon->boundingRect();
  iconRect.moveTo(m_pIcon->pos());
  iconRect.setSize(iconRect.size() * m_pIcon->scale());

  QRectF titleRect;
  {
    QPointF titlePos = m_pTitleLabel->pos();
    titlePos.setX(iconRect.right());
    m_pTitleLabel->setPos(titlePos);

    titleRect = m_pTitleLabel->boundingRect();
    titleRect.moveTo(titlePos);
  }

  m_pIcon->setPos(0, (titleRect.bottom() - iconRect.height()) / 2);

  QRectF subtitleRect;
  if (m_pSubtitleLabel->toPlainText().isEmpty() == false)
  {
    QPointF subtitlePos = m_pSubtitleLabel->pos();
    subtitlePos.setX(iconRect.right());
    m_pSubtitleLabel->setPos(subtitlePos);

    subtitleRect = m_pSubtitleLabel->boundingRect();
    subtitleRect.moveTo(m_pSubtitleLabel->pos());
  }

  int h = plMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5;

  int y = h;

  // Align inputs
  int maxInputWidth = 10;
  for (plQtPin* pQtPin : m_Inputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxInputWidth = plMath::Max(maxInputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int maxheight = y;
  y = h;

  // Align outputs
  int maxOutputWidth = 10;
  for (plQtPin* pQtPin : m_Outputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxOutputWidth = plMath::Max(maxOutputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int w = plMath::Max(maxInputWidth, 10) + plMath::Max(maxOutputWidth, 10) + 20;

  const int headerWidth = plMath::Max(titleRect.width(), subtitleRect.width()) + iconRect.width();
  w = plMath::Max(w, headerWidth);


  maxheight = plMath::Max(maxheight, y);

  // Align outputs to the right
  for (plUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    auto rectPin = m_Outputs[i]->GetPinRect();
    m_Outputs[i]->setX(w - rectPin.width());
  }

  m_HeaderRect = QRectF(-5, -5, w + 10, plMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5);

  {
    QPainterPath p;
    p.addRect(-5, -3, w + 10, maxheight + 10);
    setPath(p);
  }
}

void plQtNode::UpdateState()
{
  auto& typeAccessor = m_pObject->GetTypeAccessor();

  plVariant name = typeAccessor.GetValue("Name");
  if (name.IsA<plString>() && name.Get<plString>().IsEmpty() == false)
  {
    m_pTitleLabel->setPlainText(name.Get<plString>().GetData());
  }
  else
  {
    plStringBuilder tmp;
    m_pTitleLabel->setPlainText(plTranslate(typeAccessor.GetType()->GetTypeName().GetData(tmp)));
  }
}

void plQtNode::SetActive(bool active)
{
  if (m_bIsActive != active)
  {
    m_bIsActive = active;

    for (auto pInputPin : m_Inputs)
    {
      pInputPin->SetActive(active);
    }

    for (auto pOutputPin : m_Outputs)
    {
      pOutputPin->SetActive(active);
    }
  }

  update();
}

void plQtNode::CreatePins()
{
  for (auto pQtPin : m_Inputs)
  {
    delete pQtPin;
  }
  m_Inputs.Clear();

  for (auto pQtPin : m_Outputs)
  {
    delete pQtPin;
  }
  m_Outputs.Clear();

  auto inputs = m_pManager->GetInputPins(m_pObject);
  for (auto& pPinTarget : inputs)
  {
    plQtPin* pQtPin = plQtNodeScene::GetPinFactory().CreateObject(pPinTarget->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new plQtPin();
    }
    pQtPin->setParentItem(this);
    m_Inputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinTarget);
  }

  auto outputs = m_pManager->GetOutputPins(m_pObject);
  for (auto& pPinSource : outputs)
  {
    plQtPin* pQtPin = plQtNodeScene::GetPinFactory().CreateObject(pPinSource->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new plQtPin();
    }

    pQtPin->setParentItem(this);
    m_Outputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinSource);
  }
}

plQtPin* plQtNode::GetInputPin(const plPin& pin)
{
  for (plQtPin* pQtPin : m_Inputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

plQtPin* plQtNode::GetOutputPin(const plPin& pin)
{
  for (plQtPin* pQtPin : m_Outputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

plBitflags<plNodeFlags> plQtNode::GetFlags() const
{
  return m_DirtyFlags;
}

void plQtNode::ResetFlags()
{
  m_DirtyFlags = plNodeFlags::UpdateTitle;
}

void plQtNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  if (m_DirtyFlags.IsSet(plNodeFlags::UpdateTitle))
  {
    UpdateState();
    UpdateGeometry();
    m_DirtyFlags.Remove(plNodeFlags::UpdateTitle);
  }

  auto palette = QApplication::palette();

  // Draw background
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(brush());
  painter->drawPath(path());

  QColor headerColor = m_HeaderColor;
  if (!m_bIsActive)
    headerColor.setAlpha(50);

  // Draw separator
  {
    QColor separatorColor = pen().color();
    separatorColor.setAlphaF(headerColor.alphaF() * 0.5f);
    QPen p = pen();
    p.setColor(separatorColor);
    painter->setPen(p);
    painter->drawLine(m_HeaderRect.bottomLeft() + QPointF(2, 0), m_HeaderRect.bottomRight() - QPointF(2, 0));
  }

  // Draw header
  QLinearGradient headerGradient(m_HeaderRect.topLeft(), m_HeaderRect.bottomLeft());
  headerGradient.setColorAt(0.0f, headerColor);
  headerGradient.setColorAt(1.0f, headerColor.darker(120));

  painter->setClipPath(path());
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(headerGradient);
  painter->drawRect(m_HeaderRect);
  painter->setClipping(false);

  QColor labelColor;

  // Draw outline
  if (isSelected())
  {
    QPen p = pen();
    p.setColor(palette.highlight().color());
    painter->setPen(p);

    labelColor = palette.highlightedText().color();
  }
  else
  {
  //   painter->setPen(pen());
  //
     labelColor = palette.buttonText().color();
  }

  // Label
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  const bool bBackgroundIsLight = m_HeaderColor.lightnessF() > 0.6f;
  if (bBackgroundIsLight)
  {
    labelColor.setRed(255 - labelColor.red());
    labelColor.setGreen(255 - labelColor.green());
    labelColor.setBlue(255 - labelColor.blue());
  }

  m_pTitleLabel->setDefaultTextColor(labelColor);
  m_pSubtitleLabel->setDefaultTextColor(labelColor.darker(110));

  painter->setBrush(QBrush(Qt::NoBrush));
  painter->drawPath(path());
}

QVariant plQtNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (!m_pObject)
    return QGraphicsPathItem::itemChange(change, value);

  plCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  switch (change)
  {
    case QGraphicsItem::ItemPositionHasChanged:
    {
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
        m_DirtyFlags.Add(plNodeFlags::Moved);
    }
    break;

    default:
      break;
  }
  return QGraphicsPathItem::itemChange(change, value);
}
