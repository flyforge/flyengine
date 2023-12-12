#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/TestClass/TestClass.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

class plStereoTestGameState : public plGameEngineTestGameState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStereoTestGameState, plGameEngineTestGameState);

public:
  void OverrideRenderPipeline(plTypedResourceHandle<plRenderPipelineResource> hPipeline);
};

class plStereoTestApplication : public plGameEngineTestApplication
{
public:
  using SUPER = plGameEngineTestApplication;

  plStereoTestApplication(const char* szProjectDirName);
  plPlatformProfile& GetPlatformProfile() { return m_PlatformProfile; }

protected:
  virtual plUniquePtr<plGameStateBase> CreateGameState(plWorld* pWorld) override;
};


class plStereoTest : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    HoloLensPipeline,
    DefaultPipeline
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  plInt32 m_iFrame = 0;
  plStereoTestApplication* m_pOwnApplication = nullptr;

  plUInt32 m_uiImgCompIdx = 0;
  plHybridArray<plUInt32, 8> m_ImgCompFrames;
};

#endif
