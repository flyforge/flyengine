#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRmlUiCanvas2DComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("RmlFile", GetRmlFile, SetRmlFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    PL_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new plClampValueAttribute(plVec2(0), plVec2(1))),
    PL_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new plSuffixAttribute("px"), new plMinValueTextAttribute("Auto")),
    PL_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new plDefaultValueAttribute(plVec2::MakeZero()), new plSuffixAttribute("px")),
    PL_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgRmlUiReload, OnMsgReload)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input/RmlUi"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plRmlUiCanvas2DComponent::plRmlUiCanvas2DComponent() = default;
plRmlUiCanvas2DComponent::~plRmlUiCanvas2DComponent() = default;
plRmlUiCanvas2DComponent& plRmlUiCanvas2DComponent::operator=(plRmlUiCanvas2DComponent&& rhs) = default;

void plRmlUiCanvas2DComponent::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void plRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_pContext != nullptr)
  {
    plRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void plRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();
}

void plRmlUiCanvas2DComponent::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void plRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  plVec2 viewSize = plVec2(1.0f);
  if (plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, GetWorld()))
  {
    viewSize.x = pView->GetViewport().width;
    viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = viewSize.y / m_vReferenceResolution.y;
  }

  plVec2 size = plVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = viewSize.y;
  }
  m_pContext->SetSize(plVec2U32(static_cast<plUInt32>(size.x), static_cast<plUInt32>(size.y)));

  plVec2 offset = plVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset = (viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - plVec2(1.0f));
  m_pContext->SetOffset(plVec2I32(static_cast<int>(offset.x), static_cast<int>(offset.y)));

  m_pContext->SetDpiScale(fScale);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    plVec2 mousePos;
    plInputManager::GetInputSlotState(plInputSlot_MousePositionX, &mousePos.x);
    plInputManager::GetInputSlotState(plInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - offset;
    m_pContext->UpdateInput(mousePos);
  }

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      pDataBinding->Update();
    }
  }

  m_pContext->Update();
}

void plRmlUiCanvas2DComponent::SetRmlFile(const char* szFile)
{
  plRmlUiResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plRmlUiResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetRmlResource(hResource);
}

const char* plRmlUiCanvas2DComponent::GetRmlFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void plRmlUiCanvas2DComponent::SetRmlResource(const plRmlUiResourceHandle& hResource)
{
  if (m_hResource != hResource)
  {
    m_hResource = hResource;

    if (m_pContext != nullptr)
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }

      UpdateCachedValues();
    }
  }
}

void plRmlUiCanvas2DComponent::SetOffset(const plVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void plRmlUiCanvas2DComponent::SetSize(const plVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void plRmlUiCanvas2DComponent::SetAnchorPoint(const plVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
}

void plRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void plRmlUiCanvas2DComponent::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

plUInt32 plRmlUiCanvas2DComponent::AddDataBinding(plUniquePtr<plRmlUiDataBinding>&& pDataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (pDataBinding->Initialize(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  for (plUInt32 i = 0; i < m_DataBindings.GetCount(); ++i)
  {
    if (pDataBinding == nullptr)
    {
      m_DataBindings[i] = std::move(pDataBinding);
      return i;
    }
  }

  plUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(pDataBinding));
  return uiDataBindingIndex;
}

void plRmlUiCanvas2DComponent::RemoveDataBinding(plUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

plUInt32 plRmlUiCanvas2DComponent::AddBlackboardBinding(const plSharedPtr<plBlackboard>& pBlackboard)
{
  auto pDataBinding = PL_DEFAULT_NEW(plRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void plRmlUiCanvas2DComponent::RemoveBlackboardBinding(plUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

plRmlUiContext* plRmlUiCanvas2DComponent::GetOrCreateRmlContext()
{
  if (m_pContext != nullptr)
  {
    return m_pContext;
  }

  plStringBuilder sName = "Context_";
  if (m_hResource.IsValid())
  {
    sName.Append(m_hResource.GetResourceID().GetView());
  }
  sName.AppendFormat("_{}", plArgP(this));

  m_pContext = plRmlUi::GetSingleton()->CreateContext(sName, m_vSize);

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();

  return m_pContext;
}

void plRmlUiCanvas2DComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_bAutobindBlackboards;
}

void plRmlUiCanvas2DComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_vOffset;
  s >> m_vSize;
  s >> m_vAnchorPoint;
  s >> m_bPassInput;

  if (uiVersion >= 2)
  {
    s >> m_bAutobindBlackboards;
  }
}

plResult plRmlUiCanvas2DComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PL_SUCCESS;
}

void plRmlUiCanvas2DComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::Thumbnail)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr)
  {
    plRmlUi::GetSingleton()->ExtractContext(*m_pContext, msg);
  }
}

void plRmlUiCanvas2DComponent::OnMsgReload(plMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

void plRmlUiCanvas2DComponent::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    {
      plResourceLock pResource(m_hResource, plResourceAcquireMode::BlockTillLoaded);

      if (pResource->GetScaleMode() == plRmlUiScaleMode::WithScreenSize)
      {
        m_vReferenceResolution = pResource->GetReferenceResolution();
      }
    }

    {
      plResourceLock pResource(m_hResource, plResourceAcquireMode::PointerOnly);

      pResource->m_ResourceEvents.AddEventHandler(
        [hComponent = GetHandle(), pWorld = GetWorld()](const plResourceEvent& e) {
          if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading)
          {
            pWorld->PostMessage(hComponent, plMsgRmlUiReload(), plTime::MakeZero());
          }
        },
        m_ResourceEventUnsubscriber);
    }
  }
}

void plRmlUiCanvas2DComponent::UpdateAutobinding()
{
  for (plUInt32 uiIndex : m_AutoBindings)
  {
    RemoveDataBinding(uiIndex);
  }

  m_AutoBindings.Clear();

  if (m_bAutobindBlackboards)
  {
    plHybridArray<plBlackboardComponent*, 4> blackboardComponents;

    plGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      pObject->TryGetComponentsOfBaseType(blackboardComponents);

      for (auto pBlackboardComponent : blackboardComponents)
      {
        pBlackboardComponent->EnsureInitialized();

        m_AutoBindings.PushBack(AddBlackboardBinding(pBlackboardComponent->GetBoard()));
      }

      pObject = pObject->GetParent();
    }
  }
}
