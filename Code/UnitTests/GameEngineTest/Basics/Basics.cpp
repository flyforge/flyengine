#include <GameEngineTest/GameEngineTestPCH.h>

#include "Basics.h"
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_PROCESSES)
plResult TranformProject(const char* szProjectPath, plUInt32 uiCleanVersion)
{
  plGlobalLog::AddLogWriter(&plLogWriter::Console::LogMessageHandler);
  PLASMA_SCOPE_EXIT(plGlobalLog::RemoveLogWriter(&plLogWriter::Console::LogMessageHandler));

  plStringBuilder sBinPath = plOSFile::GetApplicationDirectory();

  plStringBuilder sProjectDir;
  if (plPathUtils::IsAbsolutePath(szProjectPath))
  {
    sProjectDir = szProjectPath;
    sProjectDir.MakeCleanPath();
  }
  else
  {
    // Assume to be relative to pl root.
    sProjectDir = sBinPath;
    sProjectDir.PathParentDirectory(3);
    sProjectDir.AppendPath(szProjectPath);
    sProjectDir.MakeCleanPath();
  }

  plLog::Info("Transforming assets for project '{}'", sProjectDir);

  {
    plStringBuilder sProjectAssetDir = sProjectDir;
    sProjectAssetDir.PathParentDirectory();
    sProjectAssetDir.AppendPath("AssetCache");

    plStringBuilder sCleanFile = sProjectAssetDir;
    sCleanFile.AppendPath("CleanVersion.dat");

    plUInt32 uiTargetVersion = 0;
    plOSFile f;

    if (f.Open(sCleanFile, plFileOpenMode::Read, plFileShareMode::Default).Succeeded())
    {
      f.Read(&uiTargetVersion, sizeof(plUInt32));
      f.Close();

      plLog::Info("CleanVersion.dat exists -> project has been transformed before.");
    }

    if (uiTargetVersion != uiCleanVersion)
    {
      plLog::Info("Clean version {} != {} -> deleting asset cache.", uiTargetVersion, uiCleanVersion);

      if (plOSFile::DeleteFolder(sProjectAssetDir).Failed())
      {
        plLog::Warning("Deleting the asset cache folder failed.");
      }

      if (f.Open(sCleanFile, plFileOpenMode::Write, plFileShareMode::Default).Succeeded())
      {
        f.Write(&uiCleanVersion, sizeof(plUInt32)).IgnoreResult();
        f.Close();
      }
    }
    else
    {
      plLog::Info("Clean version {} == {}.", uiTargetVersion, uiCleanVersion);
    }
  }

  sBinPath.AppendPath("EditorProcessor.exe");
  sBinPath.MakeCleanPath();

  plStringBuilder sOutputPath = plTestFramework::GetInstance()->GetAbsOutputPath();
  {
    plStringView sProjectPath = plPathUtils::GetFileDirectory(szProjectPath);
    sProjectPath.Trim("\\/");
    plStringView sProjectName = plPathUtils::GetFileName(sProjectPath);
    sOutputPath.AppendPath("Transform");
    sOutputPath.Append(sProjectName);
    if (plOSFile::CreateDirectoryStructure(sOutputPath).Failed())
      plLog::Error("Failed to create output directory: {}", sOutputPath);
  }

  plProcessOptions opt;
  opt.m_sProcess = sBinPath;
  opt.m_Arguments.PushBack("-project");
  opt.AddArgument("\"{0}\"", sProjectDir);
  opt.m_Arguments.PushBack("-transform");
  opt.m_Arguments.PushBack("PC");
  opt.m_Arguments.PushBack("-outputDir");
  opt.AddArgument("\"{0}\"", sOutputPath);
  opt.m_Arguments.PushBack("-debug");
  opt.m_Arguments.PushBack("-AssetThumbnails");
  opt.m_Arguments.PushBack("never");
  opt.m_Arguments.PushBack("-renderer");
  opt.m_Arguments.PushBack(plGameApplication::GetActiveRenderer());



  plProcess proc;
  plLog::Info("Launching: '{0}'", sBinPath);
  plResult res = proc.Launch(opt);
  if (res.Failed())
  {
    proc.Terminate().IgnoreResult();
    plLog::Error("Failed to start process: '{0}'", sBinPath);
  }

  plTime timeout = plTime::Minutes(15);
  res = proc.WaitToFinish(timeout);
  if (res.Failed())
  {
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    plStringBuilder sDumpFile = sOutputPath;
    sDumpFile.AppendPath("Timeout.dmp");
    plMiniDumpUtils::WriteExternalProcessMiniDump(sDumpFile, proc.GetProcessID()).LogFailure();
#  endif
    proc.Terminate().IgnoreResult();
    plLog::Error("Process timeout ({1}): '{0}'", sBinPath, timeout);
    return PLASMA_FAILURE;
  }
  if (proc.GetExitCode() != 0)
  {
    plLog::Error("Process failure ({0}): ExitCode: '{1}'", sBinPath, proc.GetExitCode());
    return PLASMA_FAILURE;
  }

  plLog::Success("Executed Asset Processor to transform '{}'", szProjectPath);
  return PLASMA_SUCCESS;
}
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
PLASMA_CREATE_SIMPLE_TEST_GROUP(00_Init);

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformBase)
{
  PLASMA_TEST_BOOL(TranformProject("Data/Base/plProject", 2).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformBasics)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Basics/plProject", 2).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformParticles)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Particles/plProject", 3).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformTypeScript)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/TypeScript/plProject", 3).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformEffects)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Effects/plProject", 4).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformAnimations)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/Animations/plProject", 6).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformStateMachine)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/StateMachine/plProject", 6).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformPlatformWin)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/PlatformWin/plProject", 5).Succeeded());
}

PLASMA_CREATE_SIMPLE_TEST(00_Init, TransformXR)
{
  PLASMA_TEST_BOOL(TranformProject("Data/UnitTests/GameEngineTest/XR/plProject", 5).Succeeded());
}

#endif


static plGameEngineTestBasics s_GameEngineTestBasics;

const char* plGameEngineTestBasics::GetTestName() const
{
  return "Basic Engine Tests";
}

plGameEngineTestApplication* plGameEngineTestBasics::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication_Basics);
  return m_pOwnApplication;
}

void plGameEngineTestBasics::SetupSubTests()
{
  AddSubTest("Many Meshes", SubTests::ManyMeshes);
  AddSubTest("Skybox", SubTests::Skybox);
  AddSubTest("Debug Rendering", SubTests::DebugRendering);
  AddSubTest("Debug Rendering - No Lines", SubTests::DebugRendering2);
  AddSubTest("Load Scene", SubTests::LoadScene);
}

plResult plGameEngineTestBasics::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  if (iIdentifier == SubTests::ManyMeshes)
  {
    m_pOwnApplication->SubTestManyMeshesSetup();
    return PLASMA_SUCCESS;
  }

  if (iIdentifier == SubTests::Skybox)
  {
    m_pOwnApplication->SubTestSkyboxSetup();
    return PLASMA_SUCCESS;
  }

  if (iIdentifier == SubTests::DebugRendering || iIdentifier == SubTests::DebugRendering2)
  {
    m_pOwnApplication->SubTestDebugRenderingSetup();
    return PLASMA_SUCCESS;
  }

  if (iIdentifier == SubTests::LoadScene)
  {
    m_pOwnApplication->SubTestLoadSceneSetup();
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plGameEngineTestBasics::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (iIdentifier == SubTests::ManyMeshes)
    return m_pOwnApplication->SubTestManyMeshesExec(m_iFrame);

  if (iIdentifier == SubTests::Skybox)
    return m_pOwnApplication->SubTestSkyboxExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering)
    return m_pOwnApplication->SubTestDebugRenderingExec(m_iFrame);

  if (iIdentifier == SubTests::DebugRendering2)
    return m_pOwnApplication->SubTestDebugRenderingExec2(m_iFrame);

  if (iIdentifier == SubTests::LoadScene)
    return m_pOwnApplication->SubTestLoadSceneExec(m_iFrame);

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return plTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

plGameEngineTestApplication_Basics::plGameEngineTestApplication_Basics()
  : plGameEngineTestApplication("Basics")
{
}

void plGameEngineTestApplication_Basics::SubTestManyMeshesSetup()
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  plMeshResourceHandle hMesh = plResourceManager::LoadResource<plMeshResource>("Meshes/MissingMesh.plMesh");

  plInt32 dim = 15;

  for (plInt32 z = -dim; z <= dim; ++z)
  {
    for (plInt32 y = -dim; y <= dim; ++y)
    {
      for (plInt32 x = -dim; x <= dim; ++x)
      {
        plGameObjectDesc go;
        go.m_LocalPosition.Set(x * 5.0f, y * 5.0f, z * 5.0f);

        plGameObject* pObject;
        m_pWorld->CreateObject(go, pObject);

        plMeshComponent* pMesh;
        m_pWorld->GetOrCreateComponentManager<plMeshComponentManager>()->CreateComponent(pObject, pMesh);

        pMesh->SetMesh(hMesh);
      }
    }
  }
}

plTestAppRun plGameEngineTestApplication_Basics::SubTestManyMeshesExec(plInt32 iCurFrame)
{
  {
    auto pCamera = plDynamicCast<plGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(plCameraMode::PerspectiveFixedFovY, 100.0f, 1.0f, 1000.0f);
    plVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + plVec3(1, 0, 0), plVec3(0, 0, 1));
  }

  plResourceManager::ForceNoFallbackAcquisition(3);

  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (iCurFrame > 3)
  {
    PLASMA_TEST_IMAGE(0, 150);

    return plTestAppRun::Quit;
  }

  return plTestAppRun::Continue;
}

//////////////////////////////////////////////////////////////////////////


void plGameEngineTestApplication_Basics::SubTestSkyboxSetup()
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  plTextureCubeResourceHandle hSkybox = plResourceManager::LoadResource<plTextureCubeResource>("Textures/Cubemap/plLogo_Cube_DXT1_Mips_D.dds");
  plMeshResourceHandle hMesh = plResourceManager::LoadResource<plMeshResource>("Meshes/MissingMesh.plMesh");

  // Skybox
  {
    plGameObjectDesc go;
    go.m_LocalPosition.SetZero();

    plGameObject* pObject;
    m_pWorld->CreateObject(go, pObject);

    plSkyBoxComponent* pSkybox;
    m_pWorld->GetOrCreateComponentManager<plSkyBoxComponentManager>()->CreateComponent(pObject, pSkybox);

    pSkybox->SetCubeMap(hSkybox);
  }

  // some foreground objects
  {
    plInt32 dim = 5;

    for (plInt32 z = -dim; z <= dim; ++z)
    {
      for (plInt32 y = -dim; y <= dim; ++y)
      {
        for (plInt32 x = -dim; x <= dim; ++x)
        {
          plGameObjectDesc go;
          go.m_LocalPosition.Set(x * 10.0f, y * 10.0f, z * 10.0f);

          plGameObject* pObject;
          m_pWorld->CreateObject(go, pObject);

          plMeshComponent* pMesh;
          m_pWorld->GetOrCreateComponentManager<plMeshComponentManager>()->CreateComponent(pObject, pMesh);

          pMesh->SetMesh(hMesh);
        }
      }
    }
  }
}

plTestAppRun plGameEngineTestApplication_Basics::SubTestSkyboxExec(plInt32 iCurFrame)
{
  plResourceManager::ForceNoFallbackAcquisition(3);

  auto pCamera = plDynamicCast<plGameState*>(GetActiveGameState())->GetMainCamera();
  pCamera->SetCameraMode(plCameraMode::PerspectiveFixedFovY, 120.0f, 1.0f, 100.0f);
  plVec3 pos = plVec3(iCurFrame * 5.0f, 0, 0);
  pCamera->LookAt(pos, pos + plVec3(1, 0, 0), plVec3(0, 0, 1));
  pCamera->RotateGlobally(plAngle::Degree(0), plAngle::Degree(0), plAngle::Degree(iCurFrame * 80.0f));

  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (iCurFrame < 5)
    return plTestAppRun::Continue;

  PLASMA_TEST_IMAGE(iCurFrame - 5, 150);

  if (iCurFrame < 8)
    return plTestAppRun::Continue;

  return plTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void plGameEngineTestApplication_Basics::SubTestDebugRenderingSetup()
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  m_pWorld->Clear();

  plRenderWorld::ResetFrameCounter();
}

plTestAppRun plGameEngineTestApplication_Basics::SubTestDebugRenderingExec(plInt32 iCurFrame)
{
  {
    auto pCamera = plDynamicCast<plGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(plCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    plVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + plVec3(1, 0, 0), plVec3(0, 0, 1));
  }

  // line box
  {
    plBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(plVec3(10, -5, 1), plVec3(1, 2, 3));

    plTransform t;
    t.SetIdentity();
    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(25));
    plDebugRenderer::DrawLineBox(m_pWorld.Borrow(), bbox, plColor::HotPink, t);
  }

  // line box
  {
    plBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(plVec3(10, -3, 1), plVec3(1, 2, 3));

    plTransform t;
    t.SetIdentity();
    t.m_vPosition.Set(0, 5, -2);
    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(25));
    plDebugRenderer::DrawLineBoxCorners(m_pWorld.Borrow(), bbox, 0.5f, plColor::DeepPink, t);
  }

  // 2D Rect
  {
    plDebugRenderer::Draw2DRectangle(m_pWorld.Borrow(), plRectFloat(10, 50, 35, 15), 0.1f, plColor::LawnGreen);
  }

  // Sphere
  {
    plBoundingSphere sphere;
    sphere.SetElements(plVec3(8, -5, -4), 2);
    plDebugRenderer::DrawLineSphere(m_pWorld.Borrow(), sphere, plColor::Tomato);
  }

  // Solid box
  {
    plBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(plVec3(10, -5, 1), plVec3(1, 2, 3));

    plDebugRenderer::DrawSolidBox(m_pWorld.Borrow(), bbox, plColor::BurlyWood);
  }

  // Text
  {
    plDebugRenderer::Draw2DText(m_pWorld.Borrow(), "Not 'a test\"", plVec2I32(30, 10), plColor::AntiqueWhite, 24);
    plDebugRenderer::Draw2DText(m_pWorld.Borrow(), "!@#$%^&*()_[]{}|", plVec2I32(20, 200), plColor::AntiqueWhite, 24);
  }

  // Frustum
  {
    plFrustum f;
    f.SetFrustum(plVec3(5, 7, 3), plVec3(0, -1, 0), plVec3(0, 0, 1), plAngle::Degree(30), plAngle::Degree(20), 0.1f, 5.0f);
    plDebugRenderer::DrawLineFrustum(m_pWorld.Borrow(), f, plColor::Cornsilk);
  }

  // Lines
  {
    plHybridArray<plDebugRenderer::Line, 4> lines;
    lines.PushBack(plDebugRenderer::Line(plVec3(3, -4, -4), plVec3(4, -2, -3)));
    lines.PushBack(plDebugRenderer::Line(plVec3(4, -2, -3), plVec3(2, 2, -2)));
    plDebugRenderer::DrawLines(m_pWorld.Borrow(), lines, plColor::SkyBlue);
  }

  // Triangles
  {
    plHybridArray<plDebugRenderer::Triangle, 4> tris;
    tris.PushBack(plDebugRenderer::Triangle(plVec3(7, 0, 0), plVec3(7, 2, 0), plVec3(7, 2, 1)));
    tris.PushBack(plDebugRenderer::Triangle(plVec3(7, 3, 0), plVec3(7, 1, 0), plVec3(7, 3, 1)));
    plDebugRenderer::DrawSolidTriangles(m_pWorld.Borrow(), tris, plColor::Gainsboro);
  }

  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return plTestAppRun::Continue;

  PLASMA_TEST_IMAGE(0, 150);

  return plTestAppRun::Quit;
}

plTestAppRun plGameEngineTestApplication_Basics::SubTestDebugRenderingExec2(plInt32 iCurFrame)
{
  {
    auto pCamera = plDynamicCast<plGameState*>(GetActiveGameState())->GetMainCamera();
    pCamera->SetCameraMode(plCameraMode::PerspectiveFixedFovY, 100.0f, 0.1f, 1000.0f);
    plVec3 pos;
    pos.SetZero();
    pCamera->LookAt(pos, pos + plVec3(1, 0, 0), plVec3(0, 0, 1));
  }

  // Text
  {
    plDebugRenderer::Draw2DText(m_pWorld.Borrow(), plFmt("Frame# {}", plRenderWorld::GetFrameCounter()), plVec2I32(10, 10), plColor::AntiqueWhite, 24);
    plDebugRenderer::DrawInfoText(m_pWorld.Borrow(), plDebugTextPlacement::BottomLeft, "test", plFmt("Frame# {}", plRenderWorld::GetFrameCounter()));
    plDebugRenderer::DrawInfoText(m_pWorld.Borrow(), plDebugTextPlacement::BottomRight, "test", "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|");
  }

  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  // first frame no image is captured yet
  if (iCurFrame < 1)
    return plTestAppRun::Continue;

  PLASMA_TEST_IMAGE(0, 150);

  return plTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////

void plGameEngineTestApplication_Basics::SubTestLoadSceneSetup()
{
  plResourceManager::ForceNoFallbackAcquisition(3);
  plRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(false);

  LoadScene("Basics/AssetCache/Common/Lighting.plObjectGraph").IgnoreResult();
}

plTestAppRun plGameEngineTestApplication_Basics::SubTestLoadSceneExec(plInt32 iCurFrame)
{
  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  switch (iCurFrame)
  {
    case 1:
      PLASMA_TEST_IMAGE(0, 150);
      break;

    case 2:
      PLASMA_TEST_IMAGE(1, 150);
      return plTestAppRun::Quit;
  }

  return plTestAppRun::Continue;
}
