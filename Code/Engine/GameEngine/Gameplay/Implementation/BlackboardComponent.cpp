#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plBlackboardEntry, plNoBase, 1, plRTTIDefaultAllocator<plBlackboardEntry>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("EntryName", GetName, SetName),
    PLASMA_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new plDefaultValueAttribute(0)),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Flags", plBlackboardEntryFlags, m_Flags)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plBlackboardEntry::Serialize(plStreamWriter& stream) const
{
  stream << m_sName;
  stream << m_InitialValue;
  stream << m_Flags;

  return PLASMA_SUCCESS;
}

plResult plBlackboardEntry::Deserialize(plStreamReader& stream)
{
  stream >> m_sName;
  stream >> m_InitialValue;
  stream >> m_Flags;

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgBlackboardEntryChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgBlackboardEntryChanged, 1, plRTTIDefaultAllocator<plMsgBlackboardEntryChanged>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("EntryName", GetName, SetName),
    PLASMA_MEMBER_PROPERTY("OldValue", m_OldValue),
    PLASMA_MEMBER_PROPERTY("NewValue", m_NewValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plBlackboardComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName),
    PLASMA_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    PLASMA_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    PLASMA_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;

  PLASMA_BEGIN_MESSAGESENDERS
  {
    PLASMA_MESSAGE_SENDER(m_EntryChangedSender)
  }
  PLASMA_END_MESSAGESENDERS;

  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_FindBlackboard, In, "SearchObject", In, "BlackboardName")->AddFlags(plPropertyFlags::Const),
  }
  PLASMA_END_FUNCTIONS;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay/Logic"),
    new plColorAttribute(plColorScheme::Logic),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plBlackboardComponent::plBlackboardComponent()
  : m_pBoard(plBlackboard::Create())
{
}

plBlackboardComponent::plBlackboardComponent(plBlackboardComponent&& other) = default;
plBlackboardComponent::~plBlackboardComponent() = default;
plBlackboardComponent& plBlackboardComponent::operator=(plBlackboardComponent&& other) = default;

// static
plSharedPtr<plBlackboard> plBlackboardComponent::FindBlackboard(plGameObject* pObject, plStringView sBlackboardName /*= plStringView()*/)
{
  plTempHashedString sBlackboardNameHashed(sBlackboardName);

  plBlackboardComponent* pBlackboardComponent = nullptr;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed)
      {
        return pBlackboardComponent->GetBoard();
      }
    }

    pObject = pObject->GetParent();
  }

  if (sBlackboardName.IsEmpty() == false)
  {
    return plBlackboard::FindGlobal(sBlackboardNameHashed);
  }

  return nullptr;
}

void plBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }

  // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
  InitializeFromTemplate();
}

void plBlackboardComponent::OnDeactivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }

  SUPER::OnDeactivated();
}

void plBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void plBlackboardComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();

  s << m_hTemplate;
}

void plBlackboardComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = stream.GetStream();

  plStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  plDynamicArray<plBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (auto& entry : initialEntries)
    {
      m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
    }
  }

  if (uiVersion >= 2)
  {
    s >> m_hTemplate;
  }
}

const plSharedPtr<plBlackboard>& plBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

plSharedPtr<const plBlackboard> plBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage
  };
};

void plBlackboardComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(BCFlags::ShowDebugInfo, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool plBlackboardComponent::GetShowDebugInfo() const
{
  return GetUserFlag(BCFlags::ShowDebugInfo);
}

void plBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(plMakeDelegate(&plBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(plMakeDelegate(&plBlackboardComponent::OnEntryChanged, this));
  }
}

bool plBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void plBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_pBoard->SetName(szName);
}

const char* plBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}

void plBlackboardComponent::SetEntryValue(const char* szName, const plVariant& value)
{
  if (m_pBoard->SetEntryValue(plTempHashedString(szName), value).Failed())
  {
    plLog::Error("Can't set blackboard entry '{}', because it doesn't exist.", szName);
  }
}

plVariant plBlackboardComponent::GetEntryValue(const char* szName) const
{
  return m_pBoard->GetEntryValue(plTempHashedString(szName));
}

void plBlackboardComponent::SetTemplateFile(const char* szName)
{
  plBlackboardTemplateResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szName))
  {
    hResource = plResourceManager::LoadResource<plBlackboardTemplateResource>(szName);
  }

  m_hTemplate = hResource;
}

const char* plBlackboardComponent::GetTemplateFile() const
{
  if (m_hTemplate.IsValid())
  {
    return m_hTemplate.GetResourceID();
  }

  return "";
}

plUInt32 plBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const plBlackboardEntry& plBlackboardComponent::Entries_GetValue(plUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void plBlackboardComponent::Entries_SetValue(plUInt32 uiIndex, const plBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);

  if (const plBlackboard::Entry* pEntry = m_pBoard->GetEntry(m_InitialEntries[uiIndex].m_sName))
  {
    if (m_InitialEntries[uiIndex].m_sName != entry.m_sName || pEntry->m_Flags != entry.m_Flags)
    {
      m_pBoard->UnregisterEntry(m_InitialEntries[uiIndex].m_sName);
      m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
    }
  }
  else
  {
    m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue).AssertSuccess();
  m_InitialEntries[uiIndex] = entry;
}

void plBlackboardComponent::Entries_Insert(plUInt32 uiIndex, const plBlackboardEntry& entry)
{
  m_InitialEntries.Insert(entry, uiIndex);

  m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
}

void plBlackboardComponent::Entries_Remove(plUInt32 uiIndex)
{
  auto& entry = m_InitialEntries[uiIndex];
  m_pBoard->UnregisterEntry(entry.m_sName);

  m_InitialEntries.RemoveAtAndCopy(uiIndex);
}

// static
plBlackboard* plBlackboardComponent::Reflection_FindBlackboard(plGameObject* pSearchObject, plStringView sBlackboardName)
{
  return FindBlackboard(pSearchObject, sBlackboardName).Borrow();
}

void plBlackboardComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  if (GetShowDebugInfo())
  {
    msg.AddBounds(plBoundingSphere(plVec3::ZeroVector(), 2.0f), plDefaultSpatialDataCategories::RenderDynamic);
  }
}

void plBlackboardComponent::OnExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo())
    return;

  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  auto& entries = m_pBoard->GetAllEntries();
  if (entries.IsEmpty())
    return;

  plStringBuilder sb;
  sb.Append(m_pBoard->GetName(), "\n");

  for (auto it = entries.GetIterator(); it.IsValid(); ++it)
  {
    sb.AppendFormat("{}: {}\n", it.Key(), it.Value().m_Value);
  }

  plDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), plColor::Orange);
}

void plBlackboardComponent::OnEntryChanged(const plBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  plMsgBlackboardEntryChanged msg;
  msg.m_sName = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}

void plBlackboardComponent::InitializeFromTemplate()
{
  if (!m_hTemplate.IsValid())
    return;

  plResourceLock<plBlackboardTemplateResource> pTemplate(m_hTemplate, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
  }
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_BlackboardComponent);

