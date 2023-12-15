#include <GameEngineTest/GameEngineTestPCH.h>

#include "EffectsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Components/ParticleComponent.h>

static plGameEngineTestEffects s_GameEngineTestEffects;

const char* plGameEngineTestEffects::GetTestName() const
{
  return "Effects Tests";
}

plGameEngineTestApplication* plGameEngineTestEffects::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication, "Effects");
  return m_pOwnApplication;
}

void plGameEngineTestEffects::SetupSubTests()
{
  AddSubTest("Decals", SubTests::Decals);
  AddSubTest("Heightfield", SubTests::Heightfield);
  AddSubTest("WindClothRopes", SubTests::WindClothRopes);
  AddSubTest("Reflections", SubTests::Reflections);
  AddSubTest("StressTest", SubTests::StressTest);
  AddSubTest("AdvancedMeshes", SubTests::AdvancedMeshes);
}

plResult plGameEngineTestEffects::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  switch (iIdentifier)
  {
    case SubTests::Decals:
    {
      m_ImgCompFrames.PushBack({5});
      m_ImgCompFrames.PushBack({30});
      m_ImgCompFrames.PushBack({60});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Decals.plObjectGraph");
    }

    case SubTests::Heightfield:
    {
      m_ImgCompFrames.PushBack({20});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Heightfield.plObjectGraph");
    }

    case SubTests::WindClothRopes:
    {
      m_ImgCompFrames.PushBack({20, 550});
      m_ImgCompFrames.PushBack({100, 600});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Wind.plObjectGraph");
    }

    case SubTests::Reflections:
    {
      m_ImgCompFrames.PushBack({30});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Reflections.plObjectGraph");
    }

    case SubTests::StressTest:
    {
      m_ImgCompFrames.PushBack({100});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/StressTest.plObjectGraph");
    }

    case SubTests::AdvancedMeshes:
    {
      m_ImgCompFrames.PushBack({20});

      return m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/AdvancedMeshes.plObjectGraph");
    }

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plGameEngineTestEffects::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx].m_uiFrame == m_iFrame)
  {
    PLASMA_TEST_IMAGE(m_uiImgCompIdx, m_ImgCompFrames[m_uiImgCompIdx].m_uiThreshold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        plProfilingSystem::ProfilingData profilingData;
        plProfilingSystem::Capture(profilingData);

        plStringBuilder sPath(":appdata/Profiling/", plApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("effectsProfiling.json");

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