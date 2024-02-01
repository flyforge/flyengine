#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>


#include <QMenu>
#include <QPainter>

namespace
{
  static plColorGammaUB CategoryColor(const char* szCategory)
  {
    plColorScheme::Enum color = plColorScheme::Green;
    if (plStringUtils::IsEqual(szCategory, "Input"))
      color = plColorScheme::Lime;
    else if (plStringUtils::IsEqual(szCategory, "Output"))
      color = plColorScheme::Cyan;
    else if (plStringUtils::IsEqual(szCategory, "Math"))
      color = plColorScheme::Blue;

    return plColorScheme::DarkUI(color);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

plQtProcGenNode::plQtProcGenNode() = default;

void plQtProcGenNode::InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject)
{
  plQtNode::InitNode(pManager, pObject);

  const plRTTI* pRtti = pObject->GetType();

  if (const plCategoryAttribute* pAttr = pRtti->GetAttributeByType<plCategoryAttribute>())
  {
    m_HeaderColor = plToQtColor(CategoryColor(pAttr->GetCategory()));
  }
}

void plQtProcGenNode::UpdateState()
{
  plStringBuilder sTitle;

  const plRTTI* pRtti = GetObject()->GetType();
  auto& typeAccessor = GetObject()->GetTypeAccessor();

  if (const plTitleAttribute* pAttr = pRtti->GetAttributeByType<plTitleAttribute>())
  {
    plStringBuilder temp;
    plStringBuilder temp2;

    plHybridArray<const plAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    sTitle = pAttr->GetTitle();

    for (const auto& pin : GetInputPins())
    {
      temp.Set("{", pin->GetPin()->GetName(), "}");

      if (pin->HasAnyConnections())
      {
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
      }
      else
      {
        temp2.Set("{Input", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, temp2);
      }
    }

    plVariant val;
    plStringBuilder sVal;
    plStringBuilder sEnumVal;

    for (const auto& prop : properties)
    {
      if (prop->GetCategory() == plPropertyCategory::Set)
      {
        sVal = "{";

        plHybridArray<plVariant, 16> values;
        typeAccessor.GetValues(prop->GetPropertyName(), values);
        for (auto& setVal : values)
        {
          if (sVal.GetElementCount() > 1)
          {
            sVal.Append(", ");
          }
          sVal.Append(setVal.ConvertTo<plString>().GetView());
        }

        sVal.Append("}");
      }
      else
      {
        val = typeAccessor.GetValue(prop->GetPropertyName());

        if (prop->GetSpecificType()->IsDerivedFrom<plEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<plBitflagsBase>())
        {
          plReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<plInt64>(), sEnumVal);
          sVal = plTranslate(sEnumVal);
        }
        else if (prop->GetSpecificType() == plGetStaticRTTI<bool>())
        {
          sVal = val.Get<bool>() ? "[x]" : "[ ]";

          if (plStringUtils::IsEqual(prop->GetPropertyName(), "Active"))
          {
            SetActive(val.Get<bool>());
          }
        }
        else if (val.CanConvertTo<plString>())
        {
          sVal = val.ConvertTo<plString>();
        }
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }
  else
  {
    sTitle = pRtti->GetTypeName();
    if (sTitle.StartsWith_NoCase("plProcGen"))
    {
      sTitle.Shrink(9, 0);
    }
  }

  m_pTitleLabel->setPlainText(sTitle.GetData());
}

//////////////////////////////////////////////////////////////////////////

plQtProcGenPin::plQtProcGenPin() = default;
plQtProcGenPin::~plQtProcGenPin() = default;

void plQtProcGenPin::ExtendContextMenu(QMenu& ref_menu)
{
  QAction* pAction = new QAction("Debug", &ref_menu);
  pAction->setCheckable(true);
  pAction->setChecked(m_bDebug);
  pAction->connect(pAction, &QAction::triggered, [this](bool bChecked) { SetDebug(bChecked); });

  ref_menu.addAction(pAction);
}

void plQtProcGenPin::keyPressEvent(QKeyEvent* pEvent)
{
  if (pEvent->key() == Qt::Key_D || pEvent->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void plQtProcGenPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  plQtPin::paint(pPainter, pOption, pWidget);

  pPainter->save();
  pPainter->setPen(QPen(QColor(220, 0, 0), 3.5f, Qt::DotLine));
  pPainter->setBrush(Qt::NoBrush);

  if (m_bDebug)
  {
    float pad = 3.5f;
    QRectF bounds = path().boundingRect().adjusted(-pad, -pad, pad, pad);
    pPainter->drawEllipse(bounds);
  }

  pPainter->restore();
}

QRectF plQtProcGenPin::boundingRect() const
{
  QRectF bounds = plQtPin::boundingRect();
  return bounds.adjusted(-6, -6, 6, 6);
}

void plQtProcGenPin::SetDebug(bool bDebug)
{
  if (m_bDebug != bDebug)
  {
    m_bDebug = bDebug;

    auto pScene = static_cast<plQtProcGenScene*>(scene());
    pScene->SetDebugPin(bDebug ? this : nullptr);

    update();
  }
}

//////////////////////////////////////////////////////////////////////////

plQtProcGenScene::plQtProcGenScene(QObject* pParent /*= nullptr*/)
  : plQtNodeScene(pParent)
{
}

plQtProcGenScene::~plQtProcGenScene() = default;

void plQtProcGenScene::SetDebugPin(plQtProcGenPin* pDebugPin)
{
  if (m_pDebugPin == pDebugPin || m_bUpdatingDebugPin)
    return;

  if (m_pDebugPin != nullptr)
  {
    // don't recursively call this function, otherwise the resource is written twice
    // once with debug disabled, then with it enabled, and because it is so quick after each other
    // the resource manager may ignore the second update, because the first one is still ongoing
    m_bUpdatingDebugPin = true;
    m_pDebugPin->SetDebug(false);
    m_bUpdatingDebugPin = false;
  }

  m_pDebugPin = pDebugPin;

  if (plQtDocumentWindow* window = qobject_cast<plQtDocumentWindow*>(parent()))
  {
    auto document = static_cast<plProcGenGraphAssetDocument*>(window->GetDocument());
    document->SetDebugPin(pDebugPin != nullptr ? pDebugPin->GetPin() : nullptr);
  }
}

plStatus plQtProcGenScene::RemoveNode(plQtNode* pNode)
{
  auto pins = pNode->GetInputPins();
  pins.PushBackRange(pNode->GetOutputPins());

  for (auto pPin : pins)
  {
    if (pPin == m_pDebugPin)
    {
      m_pDebugPin->SetDebug(false);
    }
  }

  return plQtNodeScene::RemoveNode(pNode);
}
