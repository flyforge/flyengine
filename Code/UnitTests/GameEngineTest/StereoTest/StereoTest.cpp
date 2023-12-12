#include <GameEngineTest/GameEngineTestPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Core/World/World.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <GameEngineTest/StereoTest/StereoTest.h>
#  include <RendererCore/Components/CameraComponent.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererFoundation/Device/Device.h>

static plStereoTest s_StereoTest;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStereoTestGameState, 1, plRTTIDefaultAllocator<plStereoTestGameState>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

void plStereoTestGameState::OverrideRenderPipeline(plTypedResourceHandle<plRenderPipelineResource> hPipeline)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetRenderPipelineResource(hPipeline);
  }
}

//////////////////////////////////////////////////////////////////////////

plStereoTestApplication::plStereoTestApplication(const char* szProjectDirName)
  : plGameEngineTestApplication(szProjectDirName)
{
}

plUniquePtr<plGameStateBase> plStereoTestApplication::CreateGameState(plWorld* pWorld)
{
  return PLASMA_DEFAULT_NEW(plStereoTestGameState);
}

//////////////////////////////////////////////////////////////////////////

const char* plStereoTest::GetTestName() const
{
  return "Stereo Test";
}

plGameEngineTestApplication* plStereoTest::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plStereoTestApplication, "XR");
  return m_pOwnApplication;
}

void plStereoTest::SetupSubTests()
{
  AddSubTest("HoloLensPipeline", SubTests::HoloLensPipeline);
  AddSubTest("DefaultPipeline", SubTests::DefaultPipeline);
}

plResult plStereoTest::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::HoloLensPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    PLASMA_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.plObjectGraph"));

    auto renderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    plDynamicCast<plStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return PLASMA_SUCCESS;
  }
  if (iIdentifier == SubTests::DefaultPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    PLASMA_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.plObjectGraph"));

    auto renderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>("{ 3a5185d7-b06f-4cbd-a4d2-ddd58dd44d9d }");
    plDynamicCast<plStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plStereoTest::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    // The particle effect increases the error on lavapipe, see plGameEngineTestParticles::GetImageCompareThreshold.
    plUInt32 uiThreshhold = plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe") ? 300 : 250;
    PLASMA_TEST_IMAGE(m_uiImgCompIdx, uiThreshhold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        plProfilingSystem::ProfilingData profilingData;
        plProfilingSystem::Capture(profilingData);

        plStringBuilder sPath(":appdata/Profiling/", plApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("stereoProfiling.json");

        plFileWriter fileWriter;
        if (fileWriter.Open(sPath) == PLASMA_SUCCESS)
        {
          profilingData.Write(fileWriter).IgnoreResult();
          plLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          plLog::Error("Could not write profiling capture to '{0}'.", sPath);
        }
      }

      return plTestAppRun::Quit;
    }
  }

  return plTestAppRun::Continue;
}

#endif
