#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class plGameEngineTestApplication_Particles : public plGameEngineTestApplication
{
public:
  plGameEngineTestApplication_Particles();

  void SetupSceneSubTest(const char* szFile);
  void SetupParticleSubTest(const char* szFile);
  plTestAppRun ExecParticleSubTest(plInt32 iCurFrame);

  plUInt32 m_uiImageCompareThreshold = 110;
};

class plGameEngineTestParticles : public plGameEngineTest
{
  using SUPER = plGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual plGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    BillboardRenderer,
    ColorGradientBehavior,
    FliesBehavior,
    GravityBehavior,
    LightRenderer,
    MeshRenderer,
    RaycastBehavior,
    SizeCurveBehavior,
    TrailRenderer,
    VelocityBehavior,
    EffectRenderer,
    BoxPositionInitializer,
    SpherePositionInitializer,
    CylinderPositionInitializer,
    RandomColorInitializer,
    RandomSizeInitializer,
    RotationSpeedInitializer,
    VelocityConeInitializer,
    BurstEmitter,
    ContinuousEmitter,
    OnEventEmitter,
    QuadRotatingOrtho,
    QuadFixedEmDir,
    QuadAxisEmDir,

    Billboards,
    PullAlongBehavior,
    DistanceEmitter,
    SharedInstances,
    EventReactionEffect,
    LocalSpaceSim,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;
  plUInt32 GetImageCompareThreshold(plInt32 iIdentifier);

  plInt32 m_iFrame = 0;
  plGameEngineTestApplication_Particles* m_pOwnApplication = nullptr;
};
