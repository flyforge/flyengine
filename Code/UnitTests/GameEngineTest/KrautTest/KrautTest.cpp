#include <GameEngineTest/GameEngineTestPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include "KrautTest.h"
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <ParticlePlugin/Components/ParticleComponent.h>

static plGameEngineTestKraut s_GameEngineTestAnimations;

const char* plGameEngineTestKraut::GetTestName() const
{
  return "Kraut Tests";
}

plGameEngineTestApplication* plGameEngineTestKraut::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication, "PlatformWin");
  return m_pOwnApplication;
}

void plGameEngineTestKraut::SetupSubTests()
{
  AddSubTest("TreeRendering", SubTests::TreeRendering);
}

plResult plGameEngineTestKraut::InitializeSubTest(plInt32 iIdentifier)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::TreeRendering)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(60);

    PLASMA_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("PlatformWin/AssetCache/Common/Kraut/Kraut.plObjectGraph"));
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plTestAppRun plGameEngineTestKraut::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    PLASMA_TEST_IMAGE(m_uiImgCompIdx, 200);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return plTestAppRun::Quit;
    }
  }

  return plTestAppRun::Continue;
}

#endif
