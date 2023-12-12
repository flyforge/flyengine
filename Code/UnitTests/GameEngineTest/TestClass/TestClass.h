#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>

class plGameEngineTestGameState : public plFallbackGameState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameEngineTestGameState, plFallbackGameState);

public:
  virtual void ProcessInput() override;
  virtual plGameStatePriority DeterminePriority(plWorld* pWorld) const override;
  virtual void ConfigureInputActions() override;
};

class plGameEngineTestApplication : public plGameApplication
{
public:
  using SUPER = plGameApplication;

  plGameEngineTestApplication(const char* szProjectDirName);

  virtual plString FindProjectDirectory() const final override;
  virtual plString GetProjectDataDirectoryPath() const final override;
  const plImage& GetLastScreenshot() { return m_LastScreenshot; }

  plResult LoadScene(const char* szSceneFile);
  plWorld* GetWorld() const { return m_pWorld.Borrow(); }

protected:
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void StoreScreenshot(plImage&& image, plStringView sContext) override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual plUniquePtr<plGameStateBase> CreateGameState(plWorld* pWorld) override;

  plString m_sProjectDirName;
  plUniquePtr<plWorld> m_pWorld;
  plImage m_LastScreenshot;
};

class plGameEngineTest : public plTestBaseClass
{
  using SUPER = plTestBaseClass;

public:
  plGameEngineTest();
  ~plGameEngineTest();

  virtual plResult GetImage(plImage& ref_img) override;
  virtual plGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;

  plGameEngineTestApplication* m_pApplication = nullptr;
};
