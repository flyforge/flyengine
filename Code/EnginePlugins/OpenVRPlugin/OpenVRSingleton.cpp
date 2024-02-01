#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Stopwatch.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

#include <../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h>
#include <Core/World/World.h>
#include <GameEngine/VirtualReality/Components/StageSpaceComponent.h>
#include <RendererCore/Shader/ShaderResource.h>

PL_IMPLEMENT_SINGLETON(plOpenVR);

static plOpenVR g_OpenVRSingleton;

plOpenVR::plOpenVR()
  : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_DeviceState[0].m_mPose.SetIdentity();
}

bool plOpenVR::IsHmdPresent() const
{
  return vr::VR_IsHmdPresent();
}

bool plOpenVR::Initialize()
{
  if (m_bInitialized)
    return true;

  vr::EVRInitError eError = vr::VRInitError_None;
  m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
  if (eError != vr::VRInitError_None)
  {
    m_pHMD = nullptr;
    plLog::Error("Unable to init OpenVR runtime: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }
  m_pRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
  if (!m_pRenderModels)
  {
    m_pHMD = nullptr;
    vr::VR_Shutdown();
    plLog::Error("Unable to get OpenVR render model interface: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }

  m_bInitialized = true;
  plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(
    plMakeDelegate(&plOpenVR::GameApplicationEventHandler, this));
  plRenderWorld::s_BeginRenderEvent.AddEventHandler(plMakeDelegate(&plOpenVR::OnBeginRender, this));
  plGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(plMakeDelegate(&plOpenVR::GALDeviceEventHandler, this));
  ReadHMDInfo();

  SetStageSpace(plVRStageSpace::Standing);
  for (plVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    if (m_pHMD->IsTrackedDeviceConnected(uiDeviceID))
    {
      OnDeviceActivated(uiDeviceID);
    }
  }

  UpdatePoses();

  plLog::Success("OpenVR initialized successfully.");
  return true;
}

void plOpenVR::Deinitialize()
{
  if (m_bInitialized)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(
      plMakeDelegate(&plOpenVR::GameApplicationEventHandler, this));
    plRenderWorld::s_BeginRenderEvent.RemoveEventHandler(plMakeDelegate(&plOpenVR::OnBeginRender, this));
    plGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(plMakeDelegate(&plOpenVR::GALDeviceEventHandler, this));

    for (plVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
    {
      if (m_DeviceState[uiDeviceID].m_bDeviceIsConnected)
      {
        OnDeviceDeactivated(uiDeviceID);
      }
    }

    SetCompanionViewRenderTarget(plGALTextureHandle());
    DestroyVRView();

    vr::VR_Shutdown();
    m_pHMD = nullptr;
    m_pRenderModels = nullptr;
    m_bInitialized = false;
  }
}

bool plOpenVR::IsInitialized() const
{
  return m_bInitialized;
}

const plHMDInfo& plOpenVR::GetHmdInfo() const
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  return m_Info;
}

void plOpenVR::GetDeviceList(plHybridArray<plVRDeviceID, 64>& out_Devices) const
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  for (plVRDeviceID i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

plVRDeviceID plOpenVR::GetDeviceIDByType(plVRDeviceType::Enum type) const
{
  plVRDeviceID deviceID = -1;
  switch (type)
  {
    case plVRDeviceType::HMD:
      deviceID = 0;
      break;
    case plVRDeviceType::LeftController:
      deviceID = m_iLeftControllerDeviceID;
      break;
    case plVRDeviceType::RightController:
      deviceID = m_iRightControllerDeviceID;
      break;
    default:
      deviceID = type - plVRDeviceType::DeviceID0;
      break;
  }

  if (deviceID != -1 && !m_DeviceState[deviceID].m_bDeviceIsConnected)
  {
    deviceID = -1;
  }
  return deviceID;
}

const plVRDeviceState& plOpenVR::GetDeviceState(plVRDeviceID uiDeviceID) const
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  PL_ASSERT_DEV(uiDeviceID < vr::k_unMaxTrackedDeviceCount, "Invalid device ID.");
  PL_ASSERT_DEV(m_DeviceState[uiDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[uiDeviceID];
}

plEvent<const plVRDeviceEvent&>& plOpenVR::DeviceEvents()
{
  return m_DeviceEvents;
}

plViewHandle plOpenVR::CreateVRView(const plRenderPipelineResourceHandle& hRenderPipeline, plCamera* pCamera, plGALMSAASampleCount::Enum msaaCount)
{
  SetHMDCamera(pCamera);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plView* pMainView = nullptr;
  m_hView = plRenderWorld::CreateView("Holographic View", pMainView);
  pMainView->SetCameraUsageHint(plCameraUsageHint::MainView);

  {
    plGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, plGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hColorRT = pDevice->CreateTexture(tcd);

    // Store desc for one eye for later.
    m_eyeDesc = tcd;
    m_eyeDesc.m_uiArraySize = 1;
  }
  {
    plGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, plGALResourceFormat::DFloat, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hDepthRT = pDevice->CreateTexture(tcd);
  }

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hColorRT))
    .SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hDepthRT));

  pMainView->SetRenderTargetSetup(m_RenderTargetSetup);
  pMainView->SetRenderPipelineResource(hRenderPipeline);
  pMainView->SetCamera(&m_VRCamera);
  pMainView->SetRenderPassProperty("ColorSource", "MSAA_Mode", (plInt32)msaaCount);
  pMainView->SetRenderPassProperty("DepthStencil", "MSAA_Mode", (plInt32)msaaCount);

  pMainView->SetViewport(plRectFloat((float)m_Info.m_vEyeRenderTargetSize.x, (float)m_Info.m_vEyeRenderTargetSize.y));

  plRenderWorld::AddMainView(m_hView);
  return m_hView;
}

plViewHandle plOpenVR::GetVRView() const
{
  return m_hView;
}

bool plOpenVR::DestroyVRView()
{
  if (m_hView.IsInvalidated())
    return false;

  m_pWorld = nullptr;
  SetHMDCamera(nullptr);

  vr::VRCompositor()->ClearLastSubmittedFrame();
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plRenderWorld::RemoveMainView(m_hView);
  plRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();
  m_RenderTargetSetup.DestroyAllAttachedViews();

  pDevice->DestroyTexture(m_hColorRT);
  m_hColorRT.Invalidate();
  pDevice->DestroyTexture(m_hDepthRT);
  m_hDepthRT.Invalidate();
  return true;
}

bool plOpenVR::SupportsCompanionView()
{
  return true;
}

bool plOpenVR::SetCompanionViewRenderTarget(plGALTextureHandle hRenderTarget)
{
  if (!m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Maintain already created resources (just switch target).
  }
  else if (!m_hCompanionRenderTarget.IsInvalidated() && hRenderTarget.IsInvalidated())
  {
    // Delete companion resources.
    plRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
    m_hCompanionConstantBuffer.Invalidate();
  }
  else if (m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Create companion resources.
    m_hCompanionShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/VRCompanionView.plShader");
    PL_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = plRenderContext::CreateConstantBufferStorage<plVRCompanionViewConstants>();
    m_hCompanionRenderTarget = hRenderTarget;
  }
  return true;
}

plGALTextureHandle plOpenVR::GetCompanionViewRenderTarget() const
{
  return m_hCompanionRenderTarget;
}

void plOpenVR::GameApplicationEventHandler(const plGameApplicationExecutionEvent& e)
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (e.m_Type == plGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event)))
    {
      switch (event.eventType)
      {
        case vr::VREvent_TrackedDeviceActivated:
        {
          OnDeviceActivated((plVRDeviceID)event.trackedDeviceIndex);
        }
        break;
        case vr::VREvent_TrackedDeviceDeactivated:
        {
          OnDeviceDeactivated((plVRDeviceID)event.trackedDeviceIndex);
        }
        break;
      }
    }
  }
  else if (e.m_Type == plGameApplicationExecutionEvent::Type::BeforePresent)
  {
    if (m_hView.IsInvalidated())
      return;

    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    plGALContext* pGALContext = pDevice->GetPrimaryContext();

    plGALTextureHandle hLeft;
    plGALTextureHandle hRight;

    if (m_eyeDesc.m_SampleCount == plGALMSAASampleCount::None)
    {
      // OpenVR does not support submitting texture arrays, as there is no slice param in the VRTextureBounds_t :-/
      // However, it reads it just fine for the first slice (left eye) so we only need to copy the right eye into a
      // second texture to submit it :-)
      hLeft = m_hColorRT;
      hRight = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(m_eyeDesc);
      plGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 1;
      sourceSubRes.m_uiMipLevel = 0;

      plGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel = 0;

      pGALContext->CopyTextureRegion(hRight, destSubRes, plVec3U32(0, 0, 0), m_hColorRT, sourceSubRes,
        plBoundingBoxu32(plVec3U32(0, 0, 0), plVec3U32(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, 1)));
    }
    else
    {
      // Submitting the multi-sampled m_hColorRT will cause dx errors on submit :-/
      // So have to resolve both eyes.
      plGALTextureCreationDescription tempDesc = m_eyeDesc;
      tempDesc.m_SampleCount = plGALMSAASampleCount::None;
      hLeft = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);
      hRight = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);

      plGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 0;
      sourceSubRes.m_uiMipLevel = 0;

      plGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel = 0;
      pGALContext->ResolveTexture(hLeft, destSubRes, m_hColorRT, sourceSubRes);
      sourceSubRes.m_uiArraySlice = 1;
      pGALContext->ResolveTexture(hRight, destSubRes, m_hColorRT, sourceSubRes);
    }

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
    // TODO: We currently assume that we always use dx11 on windows. Need to figure out how to check for that.
    vr::Texture_t Texture;
    Texture.eType = vr::TextureType_DirectX;
    Texture.eColorSpace = vr::ColorSpace_Auto;

    {
      const plGALTexture* pTex = pDevice->GetTexture(hLeft);
      const plGALTextureDX11* pTex11 = static_cast<const plGALTextureDX11*>(pTex);
      Texture.handle = pTex11->GetDXTexture();
      vr::EVRCompositorError err = vr::VRCompositor()->Submit(vr::Eye_Left, &Texture, nullptr);
    }

    {
      const plGALTexture* pTex = pDevice->GetTexture(hRight);
      const plGALTextureDX11* pTex11 = static_cast<const plGALTextureDX11*>(pTex);
      Texture.handle = pTex11->GetDXTexture();
      vr::EVRCompositorError err = vr::VRCompositor()->Submit(vr::Eye_Right, &Texture, nullptr);
    }
#endif

    if (m_eyeDesc.m_SampleCount == plGALMSAASampleCount::None)
    {
      plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
    else
    {
      plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hLeft);
      plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
  }
  else if (e.m_Type == plGameApplicationExecutionEvent::Type::BeginAppTick)
  {
  }
  else if (e.m_Type == plGameApplicationExecutionEvent::Type::AfterPresent)
  {
    // This tells the compositor we submitted the frames are done rendering to them this frame.
    vr::VRCompositor()->PostPresentHandoff();

    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    plGALContext* pGALContext = pDevice->GetPrimaryContext();
    plRenderContext* m_pRenderContext = plRenderContext::GetDefaultInstance();

    if (const plGALTexture* tex = pDevice->GetTexture(m_hCompanionRenderTarget))
    {
      // We are rendering the companion window at the very start of the frame, using the content
      // of the last frame. That way we do not add additional delay before submitting the frames.
      PL_PROFILE_AND_MARKER(pGALContext, "VR CompanionView");

      m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
      m_pRenderContext->BindConstantBuffer("plVRCompanionViewConstants", m_hCompanionConstantBuffer);
      m_pRenderContext->BindShader(m_hCompanionShader);

      auto hRenderTargetView = plGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hCompanionRenderTarget);
      plVec2 targetSize = plVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

      plGALRenderTagetSetup renderTargetSetup;
      renderTargetSetup.SetRenderTarget(0, hRenderTargetView);
      pGALContext->SetRenderTargetSetup(renderTargetSetup);
      pGALContext->SetViewport(plRectFloat(targetSize.x, targetSize.y));

      auto* constants = plRenderContext::GetConstantBufferData<plVRCompanionViewConstants>(m_hCompanionConstantBuffer);
      constants->TargetSize = targetSize;

      plGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
      m_pRenderContext->BindTexture2D("VRTexture", hInputView);
      m_pRenderContext->DrawMeshBuffer();
    }
  }
}

void plOpenVR::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  if (e.m_Type == plGALDeviceEvent::Type::BeforeBeginFrame)
  {
    // Will call 'WaitGetPoses' which will block the thread. Alternatively we can
    // call 'PostPresentHandoff' but then we need to do more work ourselves.
    // According to the docu at 'PostPresentHandoff' both calls should happen
    // after present, the docu for 'WaitGetPoses' contradicts this :-/
    // This needs to happen on the render thread as OpenVR will use DX calls.
    UpdatePoses();

    // This will update the extracted view from last frame with the new data we got
    // this frame just before starting to render.
    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      pView->UpdateViewData(plRenderWorld::GetDataIndexForRendering());
    }
  }
}

void plOpenVR::OnBeginRender(plUInt64)
{
  // TODO: Ideally we would like to call UpdatePoses() here and block and in BeforeBeginFrame
  // we would predict the pose in two frames.
}

void plOpenVR::ReadHMDInfo()
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  m_Info.m_sDeviceName = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
  m_Info.m_sDeviceDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
  m_pHMD->GetRecommendedRenderTargetSize(&m_Info.m_vEyeRenderTargetSize.x, &m_Info.m_vEyeRenderTargetSize.y);
  m_Info.m_mat4eyePosLeft = GetHMDEyePose(vr::Eye_Left);
  m_Info.m_mat4eyePosRight = GetHMDEyePose(vr::Eye_Right);
}


void plOpenVR::OnDeviceActivated(plVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = true;
  switch (m_pHMD->GetTrackedDeviceClass(uiDeviceID))
  {
    case vr::TrackedDeviceClass_HMD:
      m_DeviceState[uiDeviceID].m_Type = plVRDeviceState::Type::HMD;
      break;
    case vr::TrackedDeviceClass_Controller:
      m_DeviceState[uiDeviceID].m_Type = plVRDeviceState::Type::Controller;

      break;
    case vr::TrackedDeviceClass_GenericTracker:
      m_DeviceState[uiDeviceID].m_Type = plVRDeviceState::Type::Tracker;
      break;
    case vr::TrackedDeviceClass_TrackingReference:
      m_DeviceState[uiDeviceID].m_Type = plVRDeviceState::Type::Reference;
      break;
    default:
      m_DeviceState[uiDeviceID].m_Type = plVRDeviceState::Type::Unknown;
      break;
  }

  UpdateHands();

  {
    plVRDeviceEvent e;
    e.m_Type = plVRDeviceEvent::Type::DeviceAdded;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}


void plOpenVR::OnDeviceDeactivated(plVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = false;
  m_DeviceState[uiDeviceID].m_bPoseIsValid = false;
  UpdateHands();
  {
    plVRDeviceEvent e;
    e.m_Type = plVRDeviceEvent::Type::DeviceRemoved;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}

void plOpenVR::UpdatePoses()
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  plStopwatch sw;

  UpdateHands();
  vr::TrackedDevicePose_t TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  vr::EVRCompositorError err = vr::VRCompositor()->WaitGetPoses(TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
  for (plVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    m_DeviceState[uiDeviceID].m_bPoseIsValid = TrackedDevicePose[uiDeviceID].bPoseIsValid;
    if (TrackedDevicePose[uiDeviceID].bPoseIsValid)
    {
      m_DeviceState[uiDeviceID].m_vVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vVelocity);
      m_DeviceState[uiDeviceID].m_vAngularVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vAngularVelocity);
      m_DeviceState[uiDeviceID].m_mPose = ConvertSteamVRMatrix(TrackedDevicePose[uiDeviceID].mDeviceToAbsoluteTracking);
      m_DeviceState[uiDeviceID].m_vPosition = m_DeviceState[uiDeviceID].m_mPose.GetTranslationVector();
      m_DeviceState[uiDeviceID].m_qRotation.SetFromMat3(m_DeviceState[uiDeviceID].m_mPose.GetRotationalPart());
    }
  }

  if (m_pCameraToSynchronize)
  {
    UpdateCamera();

    plMat4 viewMatrix;
    viewMatrix.SetLookAtMatrix(plVec3::ZeroVector(), plVec3(0, -1, 0), plVec3(0, 0, 1));

    plTransform add;
    add.SetIdentity();
    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      if (const plWorld* pWorld = pView->GetWorld())
      {
        PL_LOCK(pWorld->GetReadMarker());
        if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
        {
          if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            auto stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    const plMat4 mAdd = add.GetAsMat4();
    plMat4 mShiftedPos = m_DeviceState[0].m_mPose * mAdd;
    mShiftedPos.Invert();

    const plMat4 mViewTransformLeft = viewMatrix * m_Info.m_mat4eyePosLeft * mShiftedPos;
    const plMat4 mViewTransformRight = viewMatrix * m_Info.m_mat4eyePosRight * mShiftedPos;

    m_VRCamera.SetViewMatrix(mViewTransformLeft, plCameraEye::Left);
    m_VRCamera.SetViewMatrix(mViewTransformRight, plCameraEye::Right);

    // put the camera orientation into the sound listener and enable the listener override mode
    if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>("plSoundInterface"))
    {
      pSoundInterface->SetListener(
        -1, m_VRCamera.GetCenterPosition(), m_VRCamera.GetCenterDirForwards(), m_VRCamera.GetCenterDirUp(), plVec3::ZeroVector());
    }
  }
}

void plOpenVR::UpdateHands()
{
  m_iLeftControllerDeviceID = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
  m_iRightControllerDeviceID = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
}

void plOpenVR::SetStageSpace(plVRStageSpace::Enum space)
{
  m_StageSpace = space;
  switch (space)
  {
    case plVRStageSpace::Seated:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseSeated);
      break;
    case plVRStageSpace::Standing:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseStanding);
      break;
  }
}

void plOpenVR::SetHMDCamera(plCamera* pCamera)
{
  PL_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_VRCamera = *m_pCameraToSynchronize;
    m_VRCamera.SetCameraMode(plCameraMode::Stereo, 90.0f, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    UpdateCamera();
  }

  if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>("plSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(m_pCameraToSynchronize != nullptr);
  }
}


void plOpenVR::UpdateCamera()
{
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter())
  {
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.x / (float)m_Info.m_vEyeRenderTargetSize.y;
    plMat4 projLeft = GetHMDProjectionEye(vr::Hmd_Eye::Eye_Left, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    plMat4 projRight = GetHMDProjectionEye(vr::Hmd_Eye::Eye_Right, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    m_VRCamera.SetStereoProjection(projLeft, projRight, fAspectRatio);
  }
}

plMat4 plOpenVR::GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const
{
  PL_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  float Left, Right, Top, Bottom;
  m_pHMD->GetProjectionRaw(nEye, &Left, &Right, &Top, &Bottom);
  plMat4 proj;
  proj.SetPerspectiveProjectionMatrix(Left * fNear, Right * fNear, Top * fNear, Bottom * fNear, fNear, fFar);
  return proj;
}

plMat4 plOpenVR::GetHMDEyePose(vr::Hmd_Eye nEye) const
{
  PL_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
  plMat4 matrixObj = ConvertSteamVRMatrix(matEyeRight);
  matrixObj.Invert();
  return matrixObj;
}

plString plOpenVR::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError) const
{
  PL_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  const plUInt32 uiCharCount = m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
  plHybridArray<char, 128> temp;
  temp.SetCountUninitialized(uiCharCount);
  if (uiCharCount > 0)
  {
    m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, temp.GetData(), uiCharCount, peError);
  }
  else
  {
    temp.SetCount(1);
    temp[0] = 0;
  }
  return plString(temp.GetData());
}

plMat4 plOpenVR::ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose)
{
  // clang-format off
  // Convert right handed to left handed with Y and Z swapped.
  // Same as A^t * matPose * A with A being identity with y and z swapped.
  plMat4 mMat(
    matPose.m[0][0], matPose.m[0][2], matPose.m[0][1], matPose.m[0][3],
    matPose.m[2][0], matPose.m[2][2], matPose.m[2][1], matPose.m[2][3],
    matPose.m[1][0], matPose.m[1][2], matPose.m[1][1], matPose.m[1][3],
    0, 0, 0, 1.0f);

  return mMat;
  // clang-format on
}

plVec3 plOpenVR::ConvertSteamVRVector(const vr::HmdVector3_t& vector)
{
  return plVec3(vector.v[0], vector.v[2], vector.v[1]);
}

PL_STATICLINK_FILE(OpenVRPlugin, OpenVRPlugin_OpenVRSingleton);
