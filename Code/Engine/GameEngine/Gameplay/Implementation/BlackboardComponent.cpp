#include "Foundation/Serialization/AbstractObjectGraph.h"

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage,
    InitializedFromTemplate
  };
};

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plBlackboardEntry, plNoBase, 1, plRTTIDefaultAllocator<plBlackboardEntry>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new plDynamicStringEnumAttribute("BlackboardKeysEnum")),
    PL_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new plDefaultValueAttribute(0)),
    PL_BITFLAGS_MEMBER_PROPERTY("Flags", plBlackboardEntryFlags, m_Flags)
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plBlackboardEntry::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_InitialValue;
  inout_stream << m_Flags;

  return PL_SUCCESS;
}

plResult plBlackboardEntry::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_InitialValue;
  inout_stream >> m_Flags;

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_IMPLEMENT_MESSAGE_TYPE(plMsgBlackboardEntryChanged);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgBlackboardEntryChanged, 1, plRTTIDefaultAllocator<plMsgBlackboardEntryChanged>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Name", GetName, SetName),
    PL_MEMBER_PROPERTY("OldValue", m_OldValue),
    PL_MEMBER_PROPERTY("NewValue", m_NewValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plBlackboardComponent, 3)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    PL_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_FindBlackboard, In, "SearchObject", In, "BlackboardName")->AddFlags(plPropertyFlags::Const)->AddAttributes(new plFunctionArgumentAttributes(1, new plDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    PL_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name")->AddAttributes(new plFunctionArgumentAttributes(0, new plDynamicStringEnumAttribute("BlackboardKeysEnum"))),
  }
  PL_END_FUNCTIONS;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plBlackboardComponent::plBlackboardComponent() = default;
plBlackboardComponent::~plBlackboardComponent() = default;

// static
plSharedPtr<plBlackboard> plBlackboardComponent::FindBlackboard(plGameObject* pObject, plStringView sBlackboardName /*= plStringView()*/)
{
  const plTempHashedString sBlackboardNameHashed(sBlackboardName);

  plBlackboardComponent* pBlackboardComponent = nullptr;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || (pBlackboardComponent->GetBoard() && pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed))
      {
        return pBlackboardComponent->GetBoard();
      }
    }

    pObject = pObject->GetParent();
  }

  if (sBlackboardName.IsEmpty() == false)
  {
    plHashedString sHashedBlackboardName;
    sHashedBlackboardName.Assign(sBlackboardName);
    return plBlackboard::GetOrCreateGlobal(sHashedBlackboardName);
  }

  return nullptr;
}

void plBlackboardComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplate;
}

void plBlackboardComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  if (uiVersion < 3)
    return;

  plStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplate;
}

void plBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plBlackboardComponent::OnDeactivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }

  SUPER::OnDeactivated();
}

const plSharedPtr<plBlackboard>& plBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

plSharedPtr<const plBlackboard> plBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

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

void plBlackboardComponent::SetEntryValue(const char* szName, const plVariant& value)
{
  if (m_pBoard)
  {
    m_pBoard->SetEntryValue(szName, value);
  }
}

plVariant plBlackboardComponent::GetEntryValue(const char* szName) const
{
  if (m_pBoard)
  {
    return m_pBoard->GetEntryValue(plTempHashedString(szName));
  }

  return {};
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
    msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 2.0f), plDefaultSpatialDataCategories::RenderDynamic);
  }
}

void plBlackboardComponent::OnExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo() || m_pBoard == nullptr)
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

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plLocalBlackboardComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new plDynamicStringEnumAttribute("BlackboardNamesEnum")),
    PL_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    PL_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_MESSAGESENDERS
  {
    PL_MESSAGE_SENDER(m_EntryChangedSender)
  }
  PL_END_MESSAGESENDERS;
}
PL_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plLocalBlackboardComponent::plLocalBlackboardComponent()
{
  m_pBoard = plBlackboard::Create();
}

plLocalBlackboardComponent::plLocalBlackboardComponent(plLocalBlackboardComponent&& other) = default;
plLocalBlackboardComponent::~plLocalBlackboardComponent() = default;
plLocalBlackboardComponent& plLocalBlackboardComponent::operator=(plLocalBlackboardComponent&& other) = default;

void plLocalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void plLocalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void plLocalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void plLocalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void plLocalBlackboardComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void plLocalBlackboardComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  const plUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(plBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  plStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);
  m_pBoard->RemoveAllEntries();

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  plDynamicArray<plBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (auto& entry : initialEntries)
    {
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}

void plLocalBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(plMakeDelegate(&plLocalBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(plMakeDelegate(&plLocalBlackboardComponent::OnEntryChanged, this));
  }
}

bool plLocalBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void plLocalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_pBoard->SetName(szName);
}

const char* plLocalBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}


plUInt32 plLocalBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const plBlackboardEntry& plLocalBlackboardComponent::Entries_GetValue(plUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void plLocalBlackboardComponent::Entries_SetValue(plUInt32 uiIndex, const plBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);

  if (const plBlackboard::Entry* pEntry = m_pBoard->GetEntry(m_InitialEntries[uiIndex].m_sName))
  {
    if (m_InitialEntries[uiIndex].m_sName != entry.m_sName)
    {
      m_pBoard->RemoveEntry(m_InitialEntries[uiIndex].m_sName);
    }
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();

  m_InitialEntries[uiIndex] = entry;
}

void plLocalBlackboardComponent::Entries_Insert(plUInt32 uiIndex, const plBlackboardEntry& entry)
{
  m_InitialEntries.Insert(entry, uiIndex);

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
}

void plLocalBlackboardComponent::Entries_Remove(plUInt32 uiIndex)
{
  auto& entry = m_InitialEntries[uiIndex];
  m_pBoard->RemoveEntry(entry.m_sName);

  m_InitialEntries.RemoveAtAndCopy(uiIndex);
}

void plLocalBlackboardComponent::OnEntryChanged(const plBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  plMsgBlackboardEntryChanged msg;
  msg.m_sName = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}

void plLocalBlackboardComponent::InitializeFromTemplate()
{
  if (!m_hTemplate.IsValid())
    return;

  plResourceLock<plBlackboardTemplateResource> pTemplate(m_hTemplate, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plGlobalBlackboardInitMode, 1)
  PL_ENUM_CONSTANTS(plGlobalBlackboardInitMode::EnsureEntriesExist, plGlobalBlackboardInitMode::ResetEntryValues, plGlobalBlackboardInitMode::ClearEntireBlackboard)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_COMPONENT_TYPE(plGlobalBlackboardComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new plDynamicStringEnumAttribute("BlackboardNamesEnum")),
    PL_ENUM_MEMBER_PROPERTY("InitMode", plGlobalBlackboardInitMode, m_InitMode),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Logic"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plGlobalBlackboardComponent::plGlobalBlackboardComponent() = default;
plGlobalBlackboardComponent::plGlobalBlackboardComponent(plGlobalBlackboardComponent&& other) = default;
plGlobalBlackboardComponent::~plGlobalBlackboardComponent() = default;
plGlobalBlackboardComponent& plGlobalBlackboardComponent::operator=(plGlobalBlackboardComponent&& other) = default;

void plGlobalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void plGlobalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void plGlobalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void plGlobalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void plGlobalBlackboardComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_sName;
  s << m_InitMode;
}

void plGlobalBlackboardComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  const plUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(plBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_sName;
  s >> m_InitMode;
}

void plGlobalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* plGlobalBlackboardComponent::GetBlackboardName() const
{
  return m_sName;
}

void plGlobalBlackboardComponent::InitializeFromTemplate()
{
  m_pBoard = plBlackboard::GetOrCreateGlobal(m_sName);

  if (m_InitMode == plGlobalBlackboardInitMode::ClearEntireBlackboard)
  {
    m_pBoard->RemoveAllEntries();
  }

  if (!m_hTemplate.IsValid())
    return;

  plResourceLock<plBlackboardTemplateResource> pTemplate(m_hTemplate, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (!m_pBoard->HasEntry(entry.m_sName) || m_InitMode != plGlobalBlackboardInitMode::EnsureEntriesExist)
    {
      // make sure the entry exists and enforce that it has this value
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      // also overwrite the flags
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}


class plBlackboardComponent_2_3 : public plGraphPatch
{
public:
  plBlackboardComponent_2_3()
    : plGraphPatch("plBlackboardComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plLocalBlackboardComponent");
  }
};

plBlackboardComponent_2_3 g_plBlackboardComponent_2_3;


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_BlackboardComponent);
