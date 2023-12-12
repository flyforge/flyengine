#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static plGameEngineTestStateMachine s_GameEngineTestAnimations;

const char* plGameEngineTestStateMachine::GetTestName() const
{
  return "StateMachine Tests";
}

plGameEngineTestApplication* plGameEngineTestStateMachine::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication, "StateMachine");
  return m_pOwnApplication;
}

void plGameEngineTestStateMachine::SetupSubTests()
{
  AddSubTest("Builtins", SubTests::Builtins);
  AddSubTest("SimpleTransitions", SubTests::SimpleTransitions);
}

plResult plGameEngineTestStateMachine::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Builtins)
  {
    return PLASMA_SUCCESS;
  }
  else if (iIdentifier == SubTests::SimpleTransitions)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(17);
    m_ImgCompFrames.PushBack(33);
    m_ImgCompFrames.PushBack(49);

    PLASMA_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("StateMachine/AssetCache/Common/Scenes/StateMachine.plObjectGraph"));
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plGameEngineTestStateMachine::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::Builtins)
  {
    RunBuiltinsTest();
    return plTestAppRun::Quit;
  }

  const bool bVulkan = plGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  if (m_pOwnApplication->Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    PLASMA_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return plTestAppRun::Quit;
    }
  }

  return plTestAppRun::Continue;
}
