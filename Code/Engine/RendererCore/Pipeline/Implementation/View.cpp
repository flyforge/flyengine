#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plCameraUsageHint, 1)
  PL_ENUM_CONSTANT(plCameraUsageHint::None),
  PL_ENUM_CONSTANT(plCameraUsageHint::MainView),
  PL_ENUM_CONSTANT(plCameraUsageHint::EditorView),
  PL_ENUM_CONSTANT(plCameraUsageHint::RenderTarget),
  PL_ENUM_CONSTANT(plCameraUsageHint::Culling),
  PL_ENUM_CONSTANT(plCameraUsageHint::Thumbnail),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plView, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    PL_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    PL_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    PL_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    PL_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plView::plView()
{
  m_pExtractTask = PL_DEFAULT_NEW(plDelegateTask<void>, "", plTaskNesting::Never, plMakeDelegate(&plView::ExtractData, this));
}

plView::~plView() = default;

void plView::SetName(plStringView sName)
{
  m_sName.Assign(sName);

  plStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, plTaskNesting::Maybe);
}

void plView::SetWorld(plWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;

    plRenderWorld::ResetRenderDataCache(*this);
  }
}

void plView::SetSwapChain(plGALSwapChainHandle hSwapChain)
{
  if (m_Data.m_hSwapChain != hSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = hSwapChain;
    m_Data.m_renderTargets = plGALRenderTargets();
    if (m_pRenderPipeline)
    {
      plRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void plView::SetRenderTargets(const plGALRenderTargets& renderTargets)
{
  if (m_Data.m_renderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = plGALSwapChainHandle();
    m_Data.m_renderTargets = renderTargets;
    if (m_pRenderPipeline)
    {
      plRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const plGALRenderTargets& plView::GetActiveRenderTargets() const
{
  if (const plGALSwapChain* pSwapChain = plGALDevice::GetDefaultDevice()->GetSwapChain(m_Data.m_hSwapChain))
  {
    return pSwapChain->GetRenderTargets();
  }
  return m_Data.m_renderTargets;
}

void plView::SetRenderPipelineResource(plRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
  {
    return;
  }

  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline = hPipeline;

  if (m_pRenderPipeline == nullptr)
  {
    EnsureUpToDate();
  }
}

plRenderPipelineResourceHandle plView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}

void plView::SetCameraUsageHint(plEnum<plCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void plView::SetViewRenderMode(plEnum<plViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void plView::SetViewport(const plRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;

  UpdateViewData(plRenderWorld::GetDataIndexForExtraction());
}

void plView::ForceUpdate()
{
  if (m_pRenderPipeline)
  {
    plRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

void plView::ExtractData()
{
  PL_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  plRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = plRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView = this;
  extractionEvent.m_uiFrameCounter = plRenderWorld::GetFrameCounter();
  plRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);


  m_pRenderPipeline->m_sName = m_sName;
  m_pRenderPipeline->ExtractData(*this);

  extractionEvent.m_Type = plRenderWorldExtractionEvent::Type::AfterViewExtraction;
  plRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);
}

void plView::ComputeCullingFrustum(plFrustum& out_frustum) const
{
  const plCamera* pCamera = GetCullingCamera();
  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  plMat4 viewMatrix = pCamera->GetViewMatrix();

  plMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum = plFrustum::MakeFromMVP(projectionMatrix * viewMatrix);
}

void plView::SetShaderPermutationVariable(const char* szName, const char* szValue)
{
  plHashedString sName;
  sName.Assign(szName);

  for (auto& var : m_PermutationVars)
  {
    if (var.m_sName == sName)
    {
      if (var.m_sValue != szValue)
      {
        var.m_sValue.Assign(szValue);
        m_bPermutationVarsDirty = true;
      }
      return;
    }
  }

  auto& var = m_PermutationVars.ExpandAndGetRef();
  var.m_sName = sName;
  var.m_sValue.Assign(szValue);
  m_bPermutationVarsDirty = true;
}

void plView::SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const plVariant& value)
{
  SetProperty(m_PassProperties, szPassName, szPropertyName, value);
}

void plView::SetExtractorProperty(const char* szPassName, const char* szPropertyName, const plVariant& value)
{
  SetProperty(m_ExtractorProperties, szPassName, szPropertyName, value);
}

void plView::ResetRenderPassProperties()
{
  for (auto it : m_PassProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty = true;
    }
  }
}

void plView::ResetExtractorProperties()
{
  for (auto it : m_ExtractorProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty = true;
    }
  }
}

void plView::SetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName, const plVariant& value)
{
  SetReadBackProperty(m_PassReadBackProperties, szPassName, szPropertyName, value);
}

plVariant plView::GetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName)
{
  plStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  if (it.IsValid())
  {
    return it.Value().m_CurrentValue;
  }

  plLog::Warning("Unknown read-back property '{0}::{1}'", szPassName, szPropertyName);
  return plVariant();
}


bool plView::IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const
{
  plStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  return it.IsValid();
}

void plView::UpdateViewData(plUInt32 uiDataIndex)
{
  if (m_pRenderPipeline != nullptr)
  {
    m_pRenderPipeline->UpdateViewData(*this, uiDataIndex);
  }
}

void plView::UpdateCachedMatrices() const
{
  const plCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    const plMat4 viewMatrixL = pCamera->GetViewMatrix(plCameraEye::Left);
    const plMat4 viewMatrixR = pCamera->GetViewMatrix(plCameraEye::Right);

    const plMat4 invViewMatrixL = viewMatrixL.GetInverse(0.0f);
    const plMat4 invViewMatrixR = viewMatrixR.GetInverse(0.0f);

    if (m_uiLastCameraOrientationModification == 0)
    {
      m_Data.m_LastViewMatrix[0] = viewMatrixL;
      m_Data.m_LastViewMatrix[1] = viewMatrixR;

      m_Data.m_LastInverseViewMatrix[0] = invViewMatrixL;
      m_Data.m_LastInverseViewMatrix[1] = invViewMatrixR;
    }
    else
    {
      m_Data.m_LastViewMatrix[0] = m_Data.m_ViewMatrix[0];
      m_Data.m_LastViewMatrix[1] = m_Data.m_ViewMatrix[1];

      m_Data.m_LastInverseViewMatrix[0] = m_Data.m_InverseViewMatrix[0];
      m_Data.m_LastInverseViewMatrix[1] = m_Data.m_InverseViewMatrix[1];
    }

    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = viewMatrixL;
    m_Data.m_ViewMatrix[1] = viewMatrixR;

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = invViewMatrixL;
    m_Data.m_InverseViewMatrix[1] = invViewMatrixR;
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    m_fLastViewportAspectRatio = fViewportAspectRatio;
    plMat4 projectionMatrixL, projectionMatrixR;

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, projectionMatrixL, plCameraEye::Left);
    const plMat4 invProjectionMatrixL = projectionMatrixL.GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, projectionMatrixR, plCameraEye::Right);
    const plMat4 invProjectionMatrixR = projectionMatrixL.GetInverse(0.0f);

    if (m_uiLastCameraSettingsModification == 0)
    {
      m_Data.m_LastProjectionMatrix[0] = projectionMatrixL;
      m_Data.m_LastProjectionMatrix[1] = projectionMatrixR;

      m_Data.m_LastInverseProjectionMatrix[0] = invProjectionMatrixL;
      m_Data.m_LastInverseProjectionMatrix[1] = invProjectionMatrixR;
    }
    else
    {
      m_Data.m_LastProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0];
      m_Data.m_LastProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1];

      m_Data.m_LastInverseProjectionMatrix[0] = m_Data.m_InverseProjectionMatrix[0];
      m_Data.m_LastInverseProjectionMatrix[1] = m_Data.m_InverseProjectionMatrix[1];
    }

    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();

    m_Data.m_ProjectionMatrix[0] = projectionMatrixL;
    m_Data.m_ProjectionMatrix[1] = projectionMatrixR;

    m_Data.m_InverseProjectionMatrix[0] = invProjectionMatrixL;
    m_Data.m_InverseProjectionMatrix[1] = invProjectionMatrixR;
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_LastViewProjectionMatrix[i] = m_Data.m_LastProjectionMatrix[i] * m_Data.m_LastViewMatrix[i];
      m_Data.m_LastInverseViewProjectionMatrix[i] = m_Data.m_LastViewProjectionMatrix[i].GetInverse(0.0f);

      m_Data.m_ViewProjectionMatrix[i] = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void plView::EnsureUpToDate()
{
  if (m_hRenderPipeline.IsValid())
  {
    plResourceLock<plRenderPipelineResource> pPipeline(m_hRenderPipeline, plResourceAcquireMode::BlockTillLoaded);

    plUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

    if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
    {
      m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

      m_pRenderPipeline = pPipeline->CreateRenderPipeline();
      plRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());

      m_bPermutationVarsDirty = true;
      ResetAllPropertyStates(m_PassProperties);
      ResetAllPropertyStates(m_ExtractorProperties);
    }

    ApplyPermutationVars();
    ApplyRenderPassProperties();
    ApplyExtractorProperties();
  }
}

void plView::ApplyPermutationVars()
{
  if (!m_bPermutationVarsDirty)
    return;

  m_pRenderPipeline->m_PermutationVars = m_PermutationVars;
  m_bPermutationVarsDirty = false;
}

void plView::SetProperty(plMap<plString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const plVariant& value)
{
  plStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = true;
  prop.m_CurrentValue = value;
}


void plView::SetReadBackProperty(plMap<plString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const plVariant& value)
{
  plStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = false;
  prop.m_CurrentValue = value;
}

void plView::ReadBackPassProperties()
{
  plHybridArray<plRenderPipelinePass*, 16> passes;
  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    pPass->ReadBackProperties(this);
  }
}

void plView::ResetAllPropertyStates(plMap<plString, PropertyValue>& map)
{
  for (auto it = map.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bIsDirty = true;
    it.Value().m_bIsValid = true;
  }
}

void plView::ApplyRenderPassProperties()
{
  for (auto it = m_PassProperties.GetIterator(); it.IsValid(); ++it)
  {
    auto& propertyValue = it.Value();

    if (!propertyValue.m_bIsValid || !propertyValue.m_bIsDirty)
      continue;

    propertyValue.m_bIsDirty = false;

    plReflectedClass* pObject = nullptr;
    const char* szDot = propertyValue.m_sObjectName.FindSubString(".");
    if (szDot != nullptr)
    {
      PL_REPORT_FAILURE("Setting renderer properties is not possible anymore");
    }
    else
    {
      pObject = m_pRenderPipeline->GetPassByName(propertyValue.m_sObjectName);
    }

    if (pObject == nullptr)
    {
      plLog::Error("The render pass '{0}' does not exist. Property '{1}' cannot be applied.", propertyValue.m_sObjectName, propertyValue.m_sPropertyName);

      propertyValue.m_bIsValid = false;
      continue;
    }

    ApplyProperty(pObject, propertyValue, "render pass");
  }
}

void plView::ApplyExtractorProperties()
{
  for (auto it = m_ExtractorProperties.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_bIsValid || !it.Value().m_bIsDirty)
      continue;

    it.Value().m_bIsDirty = false;

    plExtractor* pExtractor = m_pRenderPipeline->GetExtractorByName(it.Value().m_sObjectName);
    if (pExtractor == nullptr)
    {
      plLog::Error("The extractor '{0}' does not exist. Property '{1}' cannot be applied.", it.Value().m_sObjectName, it.Value().m_sPropertyName);

      it.Value().m_bIsValid = false;
      continue;
    }

    ApplyProperty(pExtractor, it.Value(), "extractor");
  }
}

void plView::ApplyProperty(plReflectedClass* pObject, PropertyValue& data, const char* szTypeName)
{
  const plAbstractProperty* pAbstractProperty = pObject->GetDynamicRTTI()->FindPropertyByName(data.m_sPropertyName);
  if (pAbstractProperty == nullptr)
  {
    plLog::Error("The {0} '{1}' does not have a property called '{2}', it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  if (pAbstractProperty->GetCategory() != plPropertyCategory::Member)
  {
    plLog::Error("The {0} property '{1}::{2}' is not a member property, it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  auto pMemberProperty = static_cast<const plAbstractMemberProperty*>(pAbstractProperty);
  if (data.m_DefaultValue.IsValid() == false)
  {
    data.m_DefaultValue = plReflectionUtils::GetMemberPropertyValue(pMemberProperty, pObject);
  }

  plReflectionUtils::SetMemberPropertyValue(pMemberProperty, pObject, data.m_CurrentValue);
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
