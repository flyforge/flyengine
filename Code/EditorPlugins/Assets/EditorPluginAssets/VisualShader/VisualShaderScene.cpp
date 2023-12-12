#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>


plQtVisualShaderScene::plQtVisualShaderScene(QObject* parent)
  : plQtNodeScene(parent)
{
}

plQtVisualShaderScene::~plQtVisualShaderScene() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plQtVisualShaderPin::plQtVisualShaderPin() = default;

void plQtVisualShaderPin::SetPin(const plPin& pin)
{
  plQtPin::SetPin(pin);

  const plVisualShaderPin& shaderPin = plStaticCast<const plVisualShaderPin&>(pin);

  plStringBuilder sTooltip;
  if (!shaderPin.GetTooltip().IsEmpty())
  {
    sTooltip = shaderPin.GetTooltip();
  }
  else
  {
    sTooltip = shaderPin.GetName();
  }

  if (!shaderPin.GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", shaderPin.GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());
}

void plQtVisualShaderPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QPainterPath p = path();

  const plVisualShaderPin* pVsPin = static_cast<const plVisualShaderPin*>(GetPin());

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());

  if (pVsPin->GetType() == plPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen pen;
      pen.setColor(qRgb(255, 0, 0));
      pen.setWidth(3);
      pen.setCosmetic(true);
      pen.setStyle(Qt::PenStyle::SolidLine);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);

      painter->setPen(pen);

      painter->drawRect(this->path().boundingRect());
      painter->restore();
      return;
    }
  }

  painter->drawPath(p);
  painter->restore();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plQtVisualShaderNode::plQtVisualShaderNode() = default;

void plQtVisualShaderNode::InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject)
{
  plQtNode::InitNode(pManager, pObject);

  if (auto pDesc = plVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType()))
  {
    m_HeaderColor = plToQtColor(pDesc->m_Color);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    plLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void plQtVisualShaderNode::UpdateState()
{
  plStringBuilder temp = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("ShaderNode::"))
    temp.Shrink(12, 0);

  m_pTitleLabel->setPlainText(temp.GetData());
}
