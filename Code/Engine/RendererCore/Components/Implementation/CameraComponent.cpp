#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>


plCameraComponentManager::plCameraComponentManager(plWorld* pWorld)
  : plComponentManager<plCameraComponent, plBlockStorageType::Compact>(pWorld)
{
  plRenderWorld::s_CameraConfigsModifiedEvent.AddEventHandler(plMakeDelegate(&plCameraComponentManager::OnCameraConfigsChanged, this));
}

plCameraComponentManager::~plCameraComponentManager()
{
  plRenderWorld::s_CameraConfigsModifiedEvent.RemoveEventHandler(plMakeDelegate(&plCameraComponentManager::OnCameraConfigsChanged, this));
}

void plCameraComponentManager::Initialize()
{
  auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plCameraComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);

  plRenderWorld::s_ViewCreatedEvent.AddEventHandler(plMakeDelegate(&plCameraComponentManager::OnViewCreated, this));
}

void plCameraComponentManager::Deinitialize()
{
  plRenderWorld::s_ViewCreatedEvent.RemoveEventHandler(plMakeDelegate(&plCameraComponentManager::OnViewCreated, this));

  SUPER::Deinitialize();
}

void plCameraComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto hCameraComponent : m_ModifiedCameras)
  {
    plCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    if (plView* pView = plRenderWorld::GetViewByUsageHint(pCameraComponent->GetUsageHint(), plCameraUsageHint::None, GetWorld()))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_ModifiedCameras.Clear();

  for (auto hCameraComponent : m_RenderTargetCameras)
  {
    plCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    pCameraComponent->UpdateRenderTargetCamera();
  }

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->m_bShowStats && it->GetUsageHint() == plCameraUsageHint::MainView)
    {
      if (plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, GetWorld()))
      {
        it->ShowStats(pView);
      }
    }
  }
}

void plCameraComponentManager::ReinitializeAllRenderTargetCameras()
{
  PL_LOCK(GetWorld()->GetWriteMarker());

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->DeactivateRenderToTexture();
      it->ActivateRenderToTexture();
    }
  }
}

const plCameraComponent* plCameraComponentManager::GetCameraByUsageHint(plCameraUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

plCameraComponent* plCameraComponentManager::GetCameraByUsageHint(plCameraUsageHint::Enum usageHint)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

void plCameraComponentManager::AddRenderTargetCamera(plCameraComponent* pComponent)
{
  m_RenderTargetCameras.PushBack(pComponent->GetHandle());
}

void plCameraComponentManager::RemoveRenderTargetCamera(plCameraComponent* pComponent)
{
  m_RenderTargetCameras.RemoveAndSwap(pComponent->GetHandle());
}

void plCameraComponentManager::OnViewCreated(plView* pView)
{
  // Mark all cameras as modified so the new view gets the proper settings
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->MarkAsModified(this);
  }
}

void plCameraComponentManager::OnCameraConfigsChanged(void* dummy)
{
  ReinitializeAllRenderTargetCameras();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCameraComponent, 10, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EditorShortcut", m_iEditorShortcut)->AddAttributes(new plDefaultValueAttribute(-1), new plClampValueAttribute(-1, 9)),
    PL_ENUM_ACCESSOR_PROPERTY("UsageHint", plCameraUsageHint, GetUsageHint, SetUsageHint),
    PL_ENUM_ACCESSOR_PROPERTY("Mode", plCameraMode, GetCameraMode, SetCameraMode),
    PL_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Target", plDependencyFlags::Package)),
    PL_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new plClampValueAttribute(plVec2(0.0f), plVec2(0.9f))),
    PL_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new plDefaultValueAttribute(plVec2(1.0f)), new plClampValueAttribute(plVec2(0.1f), plVec2(1.0f))),
    PL_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.01f, 4.0f)),
    PL_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new plDefaultValueAttribute(1000.0f), new plClampValueAttribute(5.0, 10000.0f)),
    PL_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new plDefaultValueAttribute(60.0f), new plClampValueAttribute(1.0f, 170.0f)),
    PL_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.01f, 10000.0f)),
    PL_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new plTagSetWidgetAttribute("Default")),
    PL_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new plTagSetWidgetAttribute("Default")),
    PL_ACCESSOR_PROPERTY("CameraRenderPipeline", GetRenderPipelineEnum, SetRenderPipelineEnum)->AddAttributes(new plDynamicStringEnumAttribute("CameraPipelines")),
    PL_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(1.0f, 32.0f), new plSuffixAttribute(" f-stop(s)")),
    PL_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromSeconds(1.0)), new plClampValueAttribute(plTime::MakeFromSeconds(1.0f / 100000.0f), plTime::MakeFromSeconds(600.0f))),
    PL_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(50.0f, 64000.0f)),
    PL_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new plClampValueAttribute(-32.0f, 32.0f)),
    PL_MEMBER_PROPERTY("ShowStats", m_bShowStats),
    //PL_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    //PL_ACCESSOR_PROPERTY_READ_ONLY("FinalExposure", GetExposure),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 1.0f, plColor::DarkSlateBlue),
    new plCameraVisualizerAttribute("Mode", "FOV", "Dimensions", "NearPlane", "FarPlane"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCameraComponent::plCameraComponent() = default;
plCameraComponent::~plCameraComponent() = default;

void plCameraComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;

  // Version 2 till 7
  // s << m_hRenderPipeline;

  // Version 3
  s << m_fAperture;
  s << static_cast<float>(m_ShutterTime.GetSeconds());
  s << m_fISO;
  s << m_fExposureCompensation;

  // Version 4
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  // Version 6
  s << m_hRenderTarget;

  // Version 7
  s << m_vRenderTargetRectOffset;
  s << m_vRenderTargetRectSize;

  // Version 8
  s << m_sRenderPipeline;

  // Version 10
  s << m_bShowStats;
}

void plCameraComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  plCameraUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > plCameraUsageHint::MainView)
    usage = plCameraUsageHint::None;
  m_UsageHint.SetValue(usage);

  plCameraMode::StorageType cam;
  s >> cam;
  m_Mode.SetValue(cam);

  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;

  if (uiVersion >= 2 && uiVersion <= 7)
  {
    plRenderPipelineResourceHandle m_hRenderPipeline;
    s >> m_hRenderPipeline;
  }

  if (uiVersion >= 3)
  {
    s >> m_fAperture;
    float shutterTime;
    s >> shutterTime;
    m_ShutterTime = plTime::MakeFromSeconds(shutterTime);
    s >> m_fISO;
    s >> m_fExposureCompensation;
  }

  if (uiVersion >= 4)
  {
    m_IncludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
  }

  if (uiVersion >= 6)
  {
    s >> m_hRenderTarget;
  }

  if (uiVersion >= 7)
  {
    s >> m_vRenderTargetRectOffset;
    s >> m_vRenderTargetRectSize;
  }

  if (uiVersion >= 8)
  {
    s >> m_sRenderPipeline;
  }

  if (uiVersion >= 10)
  {
    s >> m_bShowStats;
  }

  MarkAsModified();
}

void plCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  // recreate everything, if the view got invalidated in between
  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  plView* pView = nullptr;
  if (!plRenderWorld::TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == plCameraMode::PerspectiveFixedFovX || m_Mode == plCameraMode::PerspectiveFixedFovY)
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  else
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);

  m_RenderTargetCamera.LookAt(
    GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(), GetOwner()->GetGlobalDirUp());
}

void plCameraComponent::ShowStats(plView* pView)
{
  if (!m_bShowStats)
    return;

  // draw stats
  {
    const plStringView sName = GetOwner()->GetName();

    plStringBuilder sb;
    sb.SetFormat("Camera '{0}':\nEV100: {1}, Exposure: {2}", sName.IsEmpty() ? pView->GetName() : sName, GetEV100(), GetExposure());
    plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::TopLeft, "CamStats", sb, plColor::White);
  }

  // draw frustum
  {
    const plGameObject* pOwner = GetOwner();
    plVec3 vPosition = pOwner->GetGlobalPosition();
    plVec3 vForward = pOwner->GetGlobalDirForwards();
    plVec3 vUp = pOwner->GetGlobalDirUp();

    const plMat4 viewMatrix = plGraphicsUtils::CreateLookAtViewMatrix(vPosition, vPosition + vForward, vUp);

    plMat4 projectionMatrix = pView->GetProjectionMatrix(plCameraEye::Left); // todo: Stereo support
    plMat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    plFrustum frustum = plFrustum::MakeFromMVP(viewProjectionMatrix);

    // TODO: limit far plane to 10 meters

    plDebugRenderer::DrawLineFrustum(GetWorld(), frustum, plColor::LimeGreen);
  }
}

void plCameraComponent::SetUsageHint(plEnum<plCameraUsageHint> val)
{
  if (val == m_UsageHint)
    return;

  DeactivateRenderToTexture();

  m_UsageHint = val;

  ActivateRenderToTexture();

  MarkAsModified();
}

void plCameraComponent::SetRenderTargetFile(const char* szFile)
{
  DeactivateRenderToTexture();

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hRenderTarget = plResourceManager::LoadResource<plRenderToTexture2DResource>(szFile);
  }
  else
  {
    m_hRenderTarget.Invalidate();
  }

  ActivateRenderToTexture();

  MarkAsModified();
}

const char* plCameraComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}

void plCameraComponent::SetRenderTargetRectOffset(plVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectOffset.x = plMath::Clamp(value.x, 0.0f, 0.9f);
  m_vRenderTargetRectOffset.y = plMath::Clamp(value.y, 0.0f, 0.9f);

  ActivateRenderToTexture();
}

void plCameraComponent::SetRenderTargetRectSize(plVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectSize.x = plMath::Clamp(value.x, 0.1f, 1.0f);
  m_vRenderTargetRectSize.y = plMath::Clamp(value.y, 0.1f, 1.0f);

  ActivateRenderToTexture();
}

void plCameraComponent::SetCameraMode(plEnum<plCameraMode> val)
{
  if (val == m_Mode)
    return;
  m_Mode = val;

  MarkAsModified();
}


void plCameraComponent::SetNearPlane(float fVal)
{
  if (fVal == m_fNearPlane)
    return;
  m_fNearPlane = fVal;

  MarkAsModified();
}


void plCameraComponent::SetFarPlane(float fVal)
{
  if (fVal == m_fFarPlane)
    return;
  m_fFarPlane = fVal;

  MarkAsModified();
}


void plCameraComponent::SetFieldOfView(float fVal)
{
  if (fVal == m_fPerspectiveFieldOfView)
    return;
  m_fPerspectiveFieldOfView = fVal;

  MarkAsModified();
}


void plCameraComponent::SetOrthoDimension(float fVal)
{
  if (fVal == m_fOrthoDimension)
    return;
  m_fOrthoDimension = fVal;

  MarkAsModified();
}

plRenderPipelineResourceHandle plCameraComponent::GetRenderPipeline() const
{
  return m_hCachedRenderPipeline;
}

plViewHandle plCameraComponent::GetRenderTargetView() const
{
  return m_hRenderTargetView;
}

const char* plCameraComponent::GetRenderPipelineEnum() const
{
  return m_sRenderPipeline.GetData();
}

void plCameraComponent::SetRenderPipelineEnum(const char* szFile)
{
  DeactivateRenderToTexture();

  m_sRenderPipeline.Assign(szFile);

  ActivateRenderToTexture();

  MarkAsModified();
}

void plCameraComponent::SetAperture(float fAperture)
{
  if (m_fAperture == fAperture)
    return;
  m_fAperture = fAperture;

  MarkAsModified();
}

void plCameraComponent::SetShutterTime(plTime shutterTime)
{
  if (m_ShutterTime == shutterTime)
    return;
  m_ShutterTime = shutterTime;

  MarkAsModified();
}

void plCameraComponent::SetISO(float fISO)
{
  if (m_fISO == fISO)
    return;
  m_fISO = fISO;

  MarkAsModified();
}

void plCameraComponent::SetExposureCompensation(float fEC)
{
  if (m_fExposureCompensation == fEC)
    return;
  m_fExposureCompensation = fEC;

  MarkAsModified();
}

float plCameraComponent::GetEV100() const
{
  // From: course_notes_moving_frostbite_to_pbr.pdf
  // EV number is defined as:
  // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
  // This gives
  // EV_s = log2 (N^2 / t)
  // EV_100 + log2 (S /100) = log2 (N^2 / t)
  // EV_100 = log2 (N^2 / t) - log2 (S /100)
  // EV_100 = log2 (N^2 / t . 100 / S)
  return plMath::Log2((m_fAperture * m_fAperture) / m_ShutterTime.AsFloatInSeconds() * 100.0f / m_fISO) - m_fExposureCompensation;
}

float plCameraComponent::GetExposure() const
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  float maxLuminance = 1.2f * plMath::Pow2(GetEV100());
  return 1.0f / maxLuminance;
}

void plCameraComponent::ApplySettingsToView(plView* pView) const
{
  if (m_UsageHint == plCameraUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == plCameraMode::OrthoFixedWidth || m_Mode == plCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  plCamera* pCamera = pView->GetCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, plMath::Max(m_fNearPlane + 0.00001f, m_fFarPlane));
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  pView->m_ExcludeTags.Set(tagEditor);

  if (m_hCachedRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hCachedRenderPipeline);
  }
}

void plCameraComponent::ResourceChangeEventHandler(const plResourceEvent& e)
{
  switch (e.m_Type)
  {
    case plResourceEvent::Type::ResourceExists:
    case plResourceEvent::Type::ResourceCreated:
      return;

    case plResourceEvent::Type::ResourceDeleted:
    case plResourceEvent::Type::ResourceContentUnloading:
    case plResourceEvent::Type::ResourceContentUpdated:
      // triggers a recreation of the view
      plRenderWorld::DeleteView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;

    default:
      break;
  }
}

void plCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetWorld()->GetComponentManager<plCameraComponentManager>()->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}


void plCameraComponent::MarkAsModified(plCameraComponentManager* pCameraManager)
{
  if (!m_bIsModified)
  {
    pCameraManager->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}

void plCameraComponent::ActivateRenderToTexture()
{
  if (m_UsageHint != plCameraUsageHint::RenderTarget)
    return;

  if (m_bRenderTargetInitialized || !m_hRenderTarget.IsValid() || m_sRenderPipeline.IsEmpty() || !IsActiveAndInitialized())
    return;

  plResourceLock<plRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pRenderTarget.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    return;
  }

  // query the render pipeline to use
  if (const auto* pConfig = plRenderWorld::FindCameraConfig(m_sRenderPipeline))
  {
    m_hCachedRenderPipeline = pConfig->m_hRenderPipeline;
  }

  if (!m_hCachedRenderPipeline.IsValid())
    return;

  m_bRenderTargetInitialized = true;

  PL_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  plStringBuilder name;
  name.SetFormat("Camera RT: {0}", GetOwner()->GetName());

  plView* pRenderTargetView = nullptr;
  m_hRenderTargetView = plRenderWorld::CreateView(name, pRenderTargetView);

  pRenderTargetView->SetRenderPipelineResource(m_hCachedRenderPipeline);

  pRenderTargetView->SetWorld(GetWorld());
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  pRenderTarget->m_ResourceEvents.AddEventHandler(plMakeDelegate(&plCameraComponent::ResourceChangeEventHandler, this));

  plGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float maxSizeX = 1.0f - m_vRenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_vRenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width = resX * plMath::Min(maxSizeX, m_vRenderTargetRectSize.x);
  const float height = resY * plMath::Min(maxSizeY, m_vRenderTargetRectSize.y);

  const float offsetX = m_vRenderTargetRectOffset.x * resX;
  const float offsetY = m_vRenderTargetRectOffset.y * resY;

  pRenderTargetView->SetViewport(plRectFloat(offsetX, offsetY, width, height));

  pRenderTarget->AddRenderView(m_hRenderTargetView);

  GetWorld()->GetComponentManager<plCameraComponentManager>()->AddRenderTargetCamera(this);
}

void plCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;
  m_hCachedRenderPipeline.Invalidate();

  PL_ASSERT_DEBUG(m_hRenderTarget.IsValid(), "Render Target should be valid");

  if (m_hRenderTarget.IsValid())
  {
    plResourceLock<plRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, plResourceAcquireMode::BlockTillLoaded);
    pRenderTarget->RemoveRenderView(m_hRenderTargetView);

    pRenderTarget->m_ResourceEvents.RemoveEventHandler(plMakeDelegate(&plCameraComponent::ResourceChangeEventHandler, this));
  }

  if (!m_hRenderTargetView.IsInvalidated())
  {
    plRenderWorld::DeleteView(m_hRenderTargetView);
    m_hRenderTargetView.Invalidate();
  }

  GetWorld()->GetComponentManager<plCameraComponentManager>()->RemoveRenderTargetCamera(this);
}

void plCameraComponent::OnActivated()
{
  SUPER::OnActivated();

  ActivateRenderToTexture();
}

void plCameraComponent::OnDeactivated()
{
  DeactivateRenderToTexture();

  SUPER::OnDeactivated();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plCameraComponentPatch_4_5 : public plGraphPatch
{
public:
  plCameraComponentPatch_4_5()
    : plGraphPatch("plCameraComponent", 5)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Usage Hint", "UsageHint");
    pNode->RenameProperty("Near Plane", "NearPlane");
    pNode->RenameProperty("Far Plane", "FarPlane");
    pNode->RenameProperty("Include Tags", "IncludeTags");
    pNode->RenameProperty("Exclude Tags", "ExcludeTags");
    pNode->RenameProperty("Render Pipeline", "RenderPipeline");
    pNode->RenameProperty("Shutter Time", "ShutterTime");
    pNode->RenameProperty("Exposure Compensation", "ExposureCompensation");
  }
};

plCameraComponentPatch_4_5 g_plCameraComponentPatch_4_5;

//////////////////////////////////////////////////////////////////////////

class plCameraComponentPatch_8_9 : public plGraphPatch
{
public:
  plCameraComponentPatch_8_9()
    : plGraphPatch("plCameraComponent", 9)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // convert the "ShutterTime" property from float to plTime
    if (auto pProp = pNode->FindProperty("ShutterTime"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float shutterTime = pProp->m_Value.Get<float>();
        pProp->m_Value = plTime::MakeFromSeconds(shutterTime);
      }
    }
  }
};

plCameraComponentPatch_8_9 g_plCameraComponentPatch_8_9;


PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_CameraComponent);
