#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, Factories)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plVisualScriptNodeRegistry);
    const plRTTI* pBaseType = plVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    plQtNodeScene::GetPinFactory().RegisterCreator(plGetStaticRTTI<plVisualScriptPin>(), [](const plRTTI* pRtti)->plQtPin* { return new plQtVisualScriptPin(); });
    /*plQtNodeScene::GetConnectionFactory().RegisterCreator(plGetStaticRTTI<plVisualScriptConnection>(), [](const plRTTI* pRtti)->plQtConnection* { return new plQtVisualScriptConnection(); });    */
    plQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const plRTTI* pRtti)->plQtNode* { return new plQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const plRTTI* pBaseType = plVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    plQtNodeScene::GetPinFactory().UnregisterCreator(plGetStaticRTTI<plVisualScriptPin>());
    //plQtNodeScene::GetConnectionFactory().UnregisterCreator(plGetStaticRTTI<plVisualScriptConnection>());
    plQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    plVisualScriptNodeRegistry* pDummy = plVisualScriptNodeRegistry::GetSingleton();
    PLASMA_DEFAULT_DELETE(pDummy);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

plQtVisualScriptPin::plQtVisualScriptPin() = default;

void plQtVisualScriptPin::SetPin(const plPin& pin)
{
  m_bTranslatePinName = false;

  plQtPin::SetPin(pin);

  UpdateTooltip();
}

bool plQtVisualScriptPin::UpdatePinColors(const plColorGammaUB* pOverwriteColor)
{
  plColorGammaUB overwriteColor;
  const plVisualScriptPin& vsPin = plStaticCast<const plVisualScriptPin&>(*GetPin());
  if (vsPin.NeedsTypeDeduction())
  {
    auto pManager = static_cast<const plVisualScriptNodeManager*>(vsPin.GetParent()->GetDocumentObjectManager());
    auto deductedType = pManager->GetDeductedType(vsPin);
    overwriteColor = plVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(deductedType);
    pOverwriteColor = &overwriteColor;
  }

  bool res = plQtPin::UpdatePinColors(pOverwriteColor);

  if (vsPin.IsRequired() && HasAnyConnections() == false)
  {
    QColor requiredColor = plToQtColor(plColorScheme::LightUI(plColorScheme::Red));

    QPen p = pen();
    p.setColor(requiredColor);
    setPen(p);

    m_pLabel->setDefaultTextColor(requiredColor);

    return true;
  }

  UpdateTooltip();

  return res;
}

void plQtVisualScriptPin::UpdateTooltip()
{
  const plVisualScriptPin& vsPin = plStaticCast<const plVisualScriptPin&>(*GetPin());

  plStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    sTooltip.Append(": ", vsPin.GetDataTypeName());

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////

plQtVisualScriptConnection::plQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

plQtVisualScriptNode::plQtVisualScriptNode() = default;

void plQtVisualScriptNode::UpdateState()
{
  plStringBuilder sTitle;

  auto pManager = static_cast<const plVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());
  auto pType = GetObject()->GetType();

  if (auto pTitleAttribute = pType->GetAttributeByType<plTitleAttribute>())
  {
    sTitle = pTitleAttribute->GetTitle();

    plHybridArray<const plAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    plStringBuilder temp;
    for (const auto& pin : GetInputPins())
    {
      if (pin->HasAnyConnections())
      {
        temp.Set("{", pin->GetPin()->GetName(), "}");
        if (static_cast<const plVisualScriptPin*>(pin->GetPin())->GetScriptDataType() == plVisualScriptDataType::String)
        {
          sTitle.ReplaceAll(temp, "");
        }
        else
        {
          sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
        }

        temp.Set("{?", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, "");
      }
    }

    plVariant val;
    plStringBuilder sVal;
    for (const auto& prop : properties)
    {
      val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

      if (prop->GetSpecificType()->IsDerivedFrom<plEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<plBitflagsBase>())
      {
        plReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<plInt64>(), sVal);
        sVal = plTranslate(sVal);
      }
      else if (val.IsA<plString>() || val.IsA<plHashedString>())
      {
        sVal = val.ConvertTo<plString>();
        if (sVal.GetCharacterCount() > 16)
        {
          sVal.Shrink(0, sVal.GetCharacterCount() - 13);
          sVal.Append("...");
        }
        sVal.Prepend("\"");
        sVal.Append("\"");
      }
      else if (val.CanConvertTo<plString>())
      {
        sVal = val.ConvertTo<plString>();
      }
      else
      {
        sVal = "<Invalid>";
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);

      temp.Set("{?", prop->GetPropertyName(), "}");
      if (val == plVariant(0))
      {
        sTitle.ReplaceAll(temp, "");
      }
      else
      {
        sTitle.ReplaceAll(temp, sVal);
      }
    }
  }
  else
  {
    sTitle = plVisualScriptNodeManager::GetNiceTypeName(GetObject());
  }

  if (const char* szSeparator = sTitle.FindSubString("::"))
  {
    m_pTitleLabel->setPlainText(szSeparator + 2);

    plStringBuilder sSubTitle = plStringView(sTitle.GetData(), szSeparator);
    sSubTitle.Trim("\"");
    m_pSubtitleLabel->setPlainText(sSubTitle.GetData());
  }
  else
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pType);
    if (pNodeDesc != nullptr && pNodeDesc->NeedsTypeDeduction())
    {
      plVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      const char* sSubTitle = deductedType != plVisualScriptDataType::Invalid ? plVisualScriptDataType::GetName(deductedType) : "Unknown";
      m_pSubtitleLabel->setPlainText(sSubTitle);
    }
  }

  auto pScene = static_cast<plQtVisualScriptNodeScene*>(scene());

  if (pManager->IsCoroutine(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetCoroutineIcon());
    m_pIcon->setScale(0.5);
  }
  else if (pManager->IsLoop(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetLoopIcon());
    m_pIcon->setScale(0.5);
  }
  else
  {
    m_pIcon->setPixmap(QPixmap());
  }
}

//////////////////////////////////////////////////////////////////////////

plQtVisualScriptNodeScene::plQtVisualScriptNodeScene(QObject* pParent /*= nullptr*/)
  : plQtNodeScene(pParent)
{
  constexpr int iconSize = 32;
  m_CoroutineIcon = QIcon(":/EditorPluginVisualScript/Coroutine.svg").pixmap(QSize(iconSize, iconSize));
  m_LoopIcon = QIcon(":/EditorPluginVisualScript/Loop.svg").pixmap(QSize(iconSize, iconSize));

  plGameObjectDocument::s_GameObjectDocumentEvents.AddEventHandler(plMakeDelegate(&plQtVisualScriptNodeScene::GameObjectDocumentEventHandler, this));
}

plQtVisualScriptNodeScene::~plQtVisualScriptNodeScene()
{
  plGameObjectDocument::s_GameObjectDocumentEvents.RemoveEventHandler(plMakeDelegate(&plQtVisualScriptNodeScene::GameObjectDocumentEventHandler, this));

  if (m_pManager != nullptr)
  {
    static_cast<const plVisualScriptNodeManager*>(m_pManager)->m_NodeChangedEvent.RemoveEventHandler(plMakeDelegate(&plQtVisualScriptNodeScene::NodeChangedHandler, this));
  }
}

void plQtVisualScriptNodeScene::InitScene(const plDocumentNodeManager* pManager)
{
  plQtNodeScene::InitScene(pManager);

  static_cast<const plVisualScriptNodeManager*>(pManager)->m_NodeChangedEvent.AddEventHandler(plMakeDelegate(&plQtVisualScriptNodeScene::NodeChangedHandler, this));
}

void plQtVisualScriptNodeScene::GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    case plGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case plGameObjectDocumentEvent::Type::GameMode_StartingExternal:
    {
      SetConnectionDecorationFlags(plQtNodeScene::ConnectionDecorationFlags::Debugging);
      break;
    }
    case plGameObjectDocumentEvent::Type::GameMode_Stopped:
    {
      SetConnectionDecorationFlags(plQtNodeScene::ConnectionDecorationFlags::Default);
      break;
    }
  }
}

void plQtVisualScriptNodeScene::NodeChangedHandler(const plDocumentObject* pObject)
{
  auto it = m_Nodes.Find(pObject);
  if (it.IsValid() == false)
    return;

  plQtNode* pNode = it.Value();

  pNode->ResetFlags();
  pNode->update();

  auto& inputPins = pNode->GetInputPins();
  for (plQtPin* pPin : inputPins)
  {
    if (static_cast<plQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }

  auto& outputPins = pNode->GetOutputPins();
  for (plQtPin* pPin : outputPins)
  {
    if (static_cast<plQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }
}
