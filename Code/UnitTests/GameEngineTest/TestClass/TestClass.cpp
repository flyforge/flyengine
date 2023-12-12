#include <GameEngineTest/GameEngineTestPCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererFoundation/Device/Device.h>

plGameEngineTest::plGameEngineTest() = default;
plGameEngineTest::~plGameEngineTest() = default;

plResult plGameEngineTest::GetImage(plImage& ref_img)
{
  ref_img.ResetAndCopy(m_pApplication->GetLastScreenshot());

  return PLASMA_SUCCESS;
}

plResult plGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return PLASMA_FAILURE;


  PLASMA_SUCCEED_OR_RETURN(plRun_Startup(m_pApplication));

  if (plGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11"))
  {
    if (plGALDevice::HasDefaultDevice() && (plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics")))
    {
      // Use different images for comparison when running the D3D11 Reference Device
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (plGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && plGALDevice::HasDefaultDevice() && plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
    }
    else
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (plGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan"))
  {
    if (plGALDevice::HasDefaultDevice() && plGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else
    {
      plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }

  return PLASMA_SUCCESS;
}

plResult plGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->RequestQuit();

    plInt32 iSteps = 2;
    while (m_pApplication->Run() == plApplication::Execution::Continue && iSteps > 0)
    {
      --iSteps;
    }

    plRun_Shutdown(m_pApplication);

    PLASMA_DEFAULT_DELETE(m_pApplication);

    if (iSteps == 0)
      return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plGameEngineTest::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  plResourceManager::ForceNoFallbackAcquisition(3);

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


plGameEngineTestApplication::plGameEngineTestApplication(const char* szProjectDirName)
  : plGameApplication("plGameEngineTest", nullptr)
{
  m_pWorld = nullptr;
  m_sProjectDirName = szProjectDirName;
}


plString plGameEngineTestApplication::FindProjectDirectory() const
{
  return m_sAppProjectPath;
}

plString plGameEngineTestApplication::GetProjectDataDirectoryPath() const
{
  plStringBuilder sProjectPath(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath(), "/", m_sProjectDirName);
  return sProjectPath;
}

plResult plGameEngineTestApplication::LoadScene(const char* szSceneFile)
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();
  m_pWorld->GetRandomNumberGenerator().Initialize(42);     // reset the RNG
  m_pWorld->GetClock().SetAccumulatedTime(plTime::Zero()); // reset the world clock

  plFileReader file;

  if (file.Open(szSceneFile).Succeeded())
  {
    // File Header
    {
      plAssetFileHeader header;
      PLASMA_SUCCEED_OR_RETURN(header.Read(file));

      char szSceneTag[16];
      file.ReadBytes(szSceneTag, sizeof(char) * 16);

      PLASMA_ASSERT_RELEASE(plStringUtils::IsEqualN(szSceneTag, "[plBinaryScene]", 16), "The given file is not a valid scene file");
    }

    plWorldReader reader;
    PLASMA_SUCCEED_OR_RETURN(reader.ReadWorldDescription(file));
    reader.InstantiateWorld(*m_pWorld, nullptr);

    return PLASMA_SUCCESS;
  }
  else
  {
    plLog::Error("Failed to load scene '{0}'", szSceneFile);
    return PLASMA_FAILURE;
  }
}

plResult plGameEngineTestApplication::BeforeCoreSystemsStartup()
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  plStringBuilder sProject;
  PLASMA_SUCCEED_OR_RETURN(plFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sProject));
  m_sAppProjectPath = sProject;

  return PLASMA_SUCCESS;
}


void plGameEngineTestApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  plStartup::StartupHighLevelSystems();

  plWorldDesc desc("GameEngineTestWorld");
  desc.m_uiRandomNumberGeneratorSeed = 42;

  m_pWorld = PLASMA_DEFAULT_NEW(plWorld, desc);
  m_pWorld->GetClock().SetFixedTimeStep(plTime::Seconds(1.0 / 30.0));

  ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
}

void plGameEngineTestApplication::BeforeHighLevelSystemsShutdown()
{
  m_pWorld = nullptr;

  SUPER::BeforeHighLevelSystemsShutdown();
}

void plGameEngineTestApplication::StoreScreenshot(plImage&& image, plStringView sContext)
{
  // store this for image comparison purposes
  m_LastScreenshot.ResetAndMove(std::move(image));
}


void plGameEngineTestApplication::Init_FileSystem_ConfigureDataDirs()
{
  SUPER::Init_FileSystem_ConfigureDataDirs();

  // additional data directories for the tests to work
  {
    plFileSystem::SetSpecialDirectory("testout", plTestFramework::GetInstance()->GetAbsOutputPath());

    plStringBuilder sBaseDir = ">sdk/Data/Base/";
    plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());

    plFileSystem::AddDataDirectory(">pltest/", "ImageComparisonDataDir", "imgout", plFileSystem::AllowWrites).IgnoreResult();
    plFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
  }
}

plUniquePtr<plGameStateBase> plGameEngineTestApplication::CreateGameState(plWorld* pWorld)
{
  return PLASMA_DEFAULT_NEW(plGameEngineTestGameState);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameEngineTestGameState, 1, plRTTIDefaultAllocator<plGameEngineTestGameState>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

void plGameEngineTestGameState::ProcessInput()
{
  // Do nothing, user input should be ignored

  // trigger taking a screenshot every frame, for image comparison purposes
  plGameApplicationBase::GetGameApplicationBaseInstance()->TakeScreenshot();
}

plGameStatePriority plGameEngineTestGameState::DeterminePriority(plWorld* pWorld) const
{
  return plGameStatePriority::Default;
}

void plGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
