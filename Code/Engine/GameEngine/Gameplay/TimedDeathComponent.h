#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgComponentInternalTrigger;
typedef plComponentManager<class plTimedDeathComponent, plBlockStorageType::Compact> plTimedDeathComponentManager;
using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;

/// \brief This component deletes the object it is attached to after a timeout.
///
/// \note The timeout must be set immediately after component creation. Once the component
/// has been initialized (start of the next frame), changing the values has no effect.
/// The only way around this, is to delete the entire component and create a new one.
class PLASMA_GAMEENGINE_DLL plTimedDeathComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plTimedDeathComponent, plComponent, plTimedDeathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  /// \brief Once this function has been executed, the timeout for deletion is fixed and cannot be reset.
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plTimedDeathComponent

public:
  plTimedDeathComponent();
  ~plTimedDeathComponent();

  plTime m_MinDelay = plTime::Seconds(1.0);   // [ property ]
  plTime m_DelayRange = plTime::Seconds(0.0); // [ property ]

  void SetTimeoutPrefab(const char* szPrefab); // [ property ]
  const char* GetTimeoutPrefab() const;        // [ property ]

protected:
  void OnTriggered(plMsgComponentInternalTrigger& msg);

  plPrefabResourceHandle m_hTimeoutPrefab; ///< Spawned when the component is killed due to the timeout
};
