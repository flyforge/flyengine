#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <AiPlugin/Navigation/Steering.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using plAiNavigationComponentManager = plComponentManagerSimple<class plAiNavigationComponent, plComponentUpdateType::WhenSimulating>;

class PL_AIPLUGIN_DLL plAiNavigationComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAiNavigationComponent, plComponent, plAiNavigationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  plAiNavMeshPathTestComponent

public:
  plAiNavigationComponent();
  ~plAiNavigationComponent();

  enum State
  {
    Idle,
    Active,
    Failed,
  };

  void SetDestination(const plVec3& vGlobalPos);
  void CancelNavigation();

  plHashedString m_sNavmeshConfig;
  plHashedString m_sPathSearchConfig;

  float m_fReachedDistance = 1.0f;
  float m_fSpeed = 5.0f;

  State GetState() const { return m_State; }

protected:
  void Update();
  void Steer(plTransform& transform);
  void PlaceOnGround(plTransform& transform);

  State m_State = State::Idle;
  plAiSteering m_Steering;
  plAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
