#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class plGameEngineTestApplication_TypeScript : public plGameEngineTestApplication
{
public:
  plGameEngineTestApplication_TypeScript();

  void SubTestBasicsSetup();
  plTestAppRun SubTestBasisExec(const char* szSubTestName);
};

class plGameEngineTestTypeScript : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

  enum SubTests
  {
    Vec2,
    Vec3,
    Quat,
    Mat3,
    Mat4,
    Transform,
    Color,
    Debug,
    GameObject,
    Component,
    Lifetime,
    Messaging,
    World,
    Utils,
  };

private:
  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  plGameEngineTestApplication_TypeScript* m_pOwnApplication = nullptr;
};
