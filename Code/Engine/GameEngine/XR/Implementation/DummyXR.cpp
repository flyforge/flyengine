#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

PLASMA_IMPLEMENT_SINGLETON(plDummyXR);

plDummyXR::plDummyXR()
  : m_SingletonRegistrar(this)
{
}

plDummyXR::~plDummyXR()
{
}

bool plDummyXR::IsHmdPresent() const
{
  return true;
}

plResult plDummyXR::Initialize()
{
  if (m_bInitialized)
    return PLASMA_FAILURE;

  m_Info.m_sDeviceName = "Dummy VR device";
  m_Info.m_vEyeRenderTargetSize = plSizeU32(640, 720);

  m_GALdeviceEventsId = plGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(plMakeDelegate(&plDummyXR::GALDeviceEventHandler, this));
  m_ExecutionEventsId = plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(plMakeDelegate(&plDummyXR::GameApplicationEventHandler, this));

  m_bInitialized = true;
  return PLASMA_SUCCESS;
}

void plDummyXR::Deinitialize()
{
  m_bInitialized = false;
  if (m_GALdeviceEventsId != 0)
  {
    plGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }
  if (m_ExecutionEventsId != 0)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }
}

bool plDummyXR::IsInitialized() const
{
  return m_bInitialized;
}

const plHMDInfo& plDummyXR::GetHmdInfo() const
{
  return m_Info;
}

plXRInputDevice& plDummyXR::GetXRInput() const
{
  return m_Input;
}

bool plDummyXR::SupportsCompanionView()
{
  return true;
}

plUniquePtr<plActor> plDummyXR::CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount, plUniquePtr<plWindowBase> companionWindow, plUniquePtr<plWindowOutputTargetGAL> companionWindowOutput)
{
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Create dummy swap chain
  {
    plGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.width, m_Info.m_vEyeRenderTargetSize.height, plGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    textureDesc.m_uiArraySize = 2;
    textureDesc.m_bAllowShaderResourceView = true;

    m_hColorRT = pDevice->CreateTexture(textureDesc);

    textureDesc.m_Format = plGALResourceFormat::D24S8;
    m_hDepthRT = pDevice->CreateTexture(textureDesc);
  }

  // SetHMDCamera
  {
    m_pCameraToSynchronize = pView->GetCamera();
    m_pCameraToSynchronize->SetCameraMode(plCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }

  plUniquePtr<plActor> pActor = PLASMA_DEFAULT_NEW(plActor, "DummyXR", this);
  PLASMA_ASSERT_DEV((companionWindow != nullptr) == (companionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");

  plUniquePtr<plActorPluginWindowXR> pActorPlugin = PLASMA_DEFAULT_NEW(plActorPluginWindowXR, this, std::move(companionWindow), std::move(companionWindowOutput));
  m_pCompanion = static_cast<plWindowOutputTargetXR*>(pActorPlugin->GetOutputTarget());

  pActor->AddPlugin(std::move(pActorPlugin));

  m_hView = pView->GetHandle();
  m_pWorld = pView->GetWorld();
  PLASMA_ASSERT_DEV(m_pWorld != nullptr, "");


  plGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = m_hColorRT;
  renderTargets.m_hDSTarget = m_hDepthRT;
  pView->SetRenderTargets(renderTargets);

  pView->SetViewport(plRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return std::move(pActor);
}

plGALTextureHandle plDummyXR::GetCurrentTexture()
{
  return m_hColorRT;
}

void plDummyXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  m_pWorld = nullptr;
  m_pCameraToSynchronize = nullptr;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  if (!m_hColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hColorRT);
    m_hColorRT.Invalidate();
  }
  if (!m_hDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hDepthRT);
    m_hDepthRT.Invalidate();
  }

  plRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();
}

void plDummyXR::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  if (e.m_Type == plGALDeviceEvent::Type::BeforeBeginFrame)
  {
  }
  else if (e.m_Type == plGALDeviceEvent::Type::BeforeEndFrame)
  {
    // Screenshots are taken during present callback so ideally we need to render the companion view before that to capture the current XR frame.
    // For backwards compatibility draw the companion view here (after present) which means that if We are in frame 100, we just rendered frame 99 (due to multi-threaded rendering) but due to this bug here we captured frame 98 for image comparison.
    // This will change once read back API is refactored to be async and will be executed at a different point in time.
    if (m_pCompanion)
    {
      // We capture the companion view in unit tests so we don't want to skip any frames.
      m_pCompanion->RenderCompanionView(false);
    }
  }
}

void plDummyXR::GameApplicationEventHandler(const plGameApplicationExecutionEvent& e)
{
  if (e.m_Type == plGameApplicationExecutionEvent::Type::BeforePresent)
  {
  }
  else if (e.m_Type == plGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    plView* pView0 = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView0))
    {
      if (plWorld* pWorld0 = pView0->GetWorld())
      {
        PLASMA_LOCK(pWorld0->GetWriteMarker());
        plCameraComponent* pCameraComponent = pWorld0->GetComponentManager<plCameraComponentManager>()->GetCameraByUsageHint(plCameraUsageHint::MainView);
        if (!pCameraComponent)
          return;

        pCameraComponent->SetCameraMode(plCameraMode::Stereo);

        // Projection
        {
          const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;

          plMat4 mProj = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(plAngle::MakeFromDegree(pCameraComponent->GetFieldOfView()), fAspectRatio,
            pCameraComponent->GetNearPlane(), plMath::Max(pCameraComponent->GetNearPlane() + 0.00001f, pCameraComponent->GetFarPlane()));

          m_pCameraToSynchronize->SetStereoProjection(mProj, mProj, fAspectRatio);
        }

        // Update camera view
        {
          plTransform add;
          add.SetIdentity();
          plView* pView = nullptr;
          if (plRenderWorld::TryGetView(m_hView, pView))
          {
            if (const plWorld* pWorld = pView->GetWorld())
            {
              PLASMA_LOCK(pWorld->GetReadMarker());
              if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
              {
                if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
                {
                  plEnum<plXRStageSpace> stageSpace = pStage->GetStageSpace();
                  if (m_StageSpace != stageSpace)
                    m_StageSpace = pStage->GetStageSpace();
                  add = pStage->GetOwner()->GetGlobalTransform();
                }
              }
            }
          }

          {
            // Update device state
            plQuat rot;
            rot.SetIdentity();
            plVec3 pos = plVec3::MakeZero();
            if (m_StageSpace == plXRStageSpace::Standing)
            {
              pos.z = m_fHeadHeight;
            }

            m_Input.m_DeviceState[0].m_vGripPosition = pos;
            m_Input.m_DeviceState[0].m_qGripRotation = rot;
            m_Input.m_DeviceState[0].m_vAimPosition = pos;
            m_Input.m_DeviceState[0].m_qAimRotation = rot;
            m_Input.m_DeviceState[0].m_Type = plXRDeviceType::HMD;
            m_Input.m_DeviceState[0].m_bGripPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bAimPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bDeviceIsConnected = true;
          }

          // Set view matrix
          {
            const float fHeight = m_StageSpace == plXRStageSpace::Standing ? m_fHeadHeight : 0.0f;
            const plMat4 mStageTransform = add.GetInverse().GetAsMat4();
            plMat4 poseLeft = plMat4::MakeTranslation(plVec3(0, -m_fEyeOffset, fHeight));
            plMat4 poseRight = plMat4::MakeTranslation(plVec3(0, m_fEyeOffset, fHeight));

            // PLASMA Forward is +X, need to add this to align the forward projection
            const plMat4 viewMatrix = plGraphicsUtils::CreateLookAtViewMatrix(plVec3::MakeZero(), plVec3(1, 0, 0), plVec3(0, 0, 1));
            const plMat4 mViewTransformLeft = viewMatrix * mStageTransform * poseLeft.GetInverse();
            const plMat4 mViewTransformRight = viewMatrix * mStageTransform * poseRight.GetInverse();

            m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, plCameraEye::Left);
            m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, plCameraEye::Right);
          }
        }
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////

void plDummyXRInput::GetDeviceList(plHybridArray<plXRDeviceID, 64>& out_Devices) const
{
  out_Devices.PushBack(0);
}

plXRDeviceID plDummyXRInput::GetDeviceIDByType(plXRDeviceType::Enum type) const
{
  plXRDeviceID deviceID = -1;
  switch (type)
  {
    case plXRDeviceType::HMD:
      deviceID = 0;
      break;
    default:
      deviceID = -1;
  }

  return deviceID;
}

const plXRDeviceState& plDummyXRInput::GetDeviceState(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(iDeviceID < 1 && iDeviceID >= 0, "Invalid device ID.");
  return m_DeviceState[iDeviceID];
}

plString plDummyXRInput::GetDeviceName(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(iDeviceID < 1 && iDeviceID >= 0, "Invalid device ID.");
  return "Dummy HMD";
}

plBitflags<plXRDeviceFeatures> plDummyXRInput::GetDeviceFeatures(plXRDeviceID iDeviceID) const
{
  PLASMA_ASSERT_DEV(iDeviceID < 1 && iDeviceID >= 0, "Invalid device ID.");
  return plXRDeviceFeatures::AimPose | plXRDeviceFeatures::GripPose;
}

void plDummyXRInput::InitializeDevice()
{
}

void plDummyXRInput::UpdateInputSlotValues()
{
}

void plDummyXRInput::RegisterInputSlots()
{
}
