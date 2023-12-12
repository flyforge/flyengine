#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class plGameEngineTestEffects : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Decals,
    Heightfield,
    WindClothRopes,
    Reflections,
    StressTest,
    AdvancedMeshes,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  plInt32 m_iFrame = 0;
  plGameEngineTestApplication* m_pOwnApplication = nullptr;

  plUInt32 m_uiImgCompIdx = 0;

  struct ImgCompare
  {
    ImgCompare(plUInt32 uiFrame, plUInt32 uiThreshold = 450)
    {
      m_uiFrame = uiFrame;
      m_uiThreshold = uiThreshold;
    }

    plUInt32 m_uiFrame;
    plUInt32 m_uiThreshold = 450;
  };

  plHybridArray<ImgCompare, 8> m_ImgCompFrames;
};
