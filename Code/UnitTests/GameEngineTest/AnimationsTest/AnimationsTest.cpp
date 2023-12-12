#include <GameEngineTest/GameEngineTestPCH.h>

#include "AnimationsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static plGameEngineTestAnimations s_GameEngineTestAnimations;

const char* plGameEngineTestAnimations::GetTestName() const
{
  return "Animations Tests";
}

plGameEngineTestApplication* plGameEngineTestAnimations::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication, "Animations");
  return m_pOwnApplication;
}

void plGameEngineTestAnimations::SetupSubTests()
{
  AddSubTest("Skeletal", SubTests::Skeletal);
}

plResult plGameEngineTestAnimations::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Skeletal)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(30);
    m_ImgCompFrames.PushBack(60);

    PLASMA_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Animations/AssetCache/Common/Scenes/AnimController.plObjectGraph"));
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plGameEngineTestAnimations::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  const bool bVulkan = plGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  if (m_pOwnApplication->Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    PLASMA_TEST_IMAGE(m_uiImgCompIdx, 300);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return plTestAppRun::Quit;
    }
  }

  return plTestAppRun::Continue;
}
