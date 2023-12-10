#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Physics/RopeSimulator.h>

//////////////////////////////////////////////////////////////////////////

class PLASMA_GAMEENGINE_DLL plFakeRopeComponentManager : public plComponentManager<class plFakeRopeComponent, plBlockStorageType::FreeList>
{
public:
  plFakeRopeComponentManager(plWorld* pWorld);
  ~plFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_GAMEENGINE_DLL plFakeRopeComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plFakeRopeComponent, plComponent, plFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plFakeRopeComponent

public:
  plFakeRopeComponent();
  ~plFakeRopeComponent();

  plUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]
  void SetAnchor1(plGameObjectHandle hActor);
  void SetAnchor2(plGameObjectHandle hActor);

  void SetSlack(float val);
  float GetSlack() const { return m_fSlack; }

  void SetAttachToAnchor1(bool bVal);
  void SetAttachToAnchor2(bool bVal);
  bool GetAttachToAnchor1() const;
  bool GetAttachToAnchor2() const;

  float m_fSlack = 0.0f;
  float m_fDamping = 0.5f;

private:
  plResult ConfigureRopeSimulator();
  void SendCurrentPose();
  void SendPreviewPose();
  void RuntimeUpdate();

  plGameObjectHandle m_hAnchor1;
  plGameObjectHandle m_hAnchor2;

  plUInt32 m_uiPreviewHash = 0;

  // if the owner or the anchor object are flagged as 'dynamic', the rope must follow their movement
  // otherwise it can skip some update steps
  bool m_bIsDynamic = true;
  plUInt8 m_uiCheckEquilibriumCounter = 0;
  plUInt8 m_uiSleepCounter = 0;
  plRopeSimulator m_RopeSim;
  float m_fWindInfluence = 0.0f;

private:
  const char* DummyGetter() const { return nullptr; }
};
