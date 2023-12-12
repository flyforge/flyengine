#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class plGameEngineTestApplication_Basics : public plGameEngineTestApplication
{
public:
  plGameEngineTestApplication_Basics();

  void SubTestManyMeshesSetup();
  plTestAppRun SubTestManyMeshesExec(plInt32 iCurFrame);

  void SubTestSkyboxSetup();
  plTestAppRun SubTestSkyboxExec(plInt32 iCurFrame);

  void SubTestDebugRenderingSetup();
  plTestAppRun SubTestDebugRenderingExec(plInt32 iCurFrame);

  plTestAppRun SubTestDebugRenderingExec2(plInt32 iCurFrame);

  void SubTestLoadSceneSetup();
  plTestAppRun SubTestLoadSceneExec(plInt32 iCurFrame);
};

class plGameEngineTestBasics : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ManyMeshes,
    Skybox,
    DebugRendering,
    DebugRendering2,
    LoadScene,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  plInt32 m_iFrame;
  plGameEngineTestApplication_Basics* m_pOwnApplication;
};
