#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class plGameEngineTestStateMachine : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Builtins,
    SimpleTransitions,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  void RunBuiltinsTest();

  plInt32 m_iFrame = 0;
  plGameEngineTestApplication* m_pOwnApplication = nullptr;

  plUInt32 m_uiImgCompIdx = 0;
  plHybridArray<plUInt32, 8> m_ImgCompFrames;
};