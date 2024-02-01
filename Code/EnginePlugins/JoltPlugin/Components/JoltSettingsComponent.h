#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <JoltPlugin/Declarations.h>

using plJoltSettingsComponentManager = plSettingsComponentManager<class plJoltSettingsComponent>;

class PL_JOLTPLUGIN_DLL plJoltSettingsComponent : public plSettingsComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltSettingsComponent, plSettingsComponent, plJoltSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltSettingsComponent

public:
  plJoltSettingsComponent();
  ~plJoltSettingsComponent();

  const plJoltSettings& GetSettings() const { return m_Settings; }

  const plVec3& GetObjectGravity() const { return m_Settings.m_vObjectGravity; } // [ property ]
  void SetObjectGravity(const plVec3& v);                                        // [ property ]

  const plVec3& GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; } // [ property ]
  void SetCharacterGravity(const plVec3& v);                                           // [ property ]

  plJoltSteppingMode::Enum GetSteppingMode() const { return m_Settings.m_SteppingMode; } // [ property ]
  void SetSteppingMode(plJoltSteppingMode::Enum mode);                                   // [ property ]

  float GetFixedFrameRate() const { return m_Settings.m_fFixedFrameRate; } // [ property ]
  void SetFixedFrameRate(float fFixedFrameRate);                           // [ property ]

  plUInt32 GetMaxSubSteps() const { return m_Settings.m_uiMaxSubSteps; } // [ property ]
  void SetMaxSubSteps(plUInt32 uiMaxSubSteps);                           // [ property ]

  plUInt32 GetMaxBodies() const { return m_Settings.m_uiMaxBodies; } // [ property ]
  void SetMaxBodies(plUInt32 uiMaxBodies);                           // [ property ]

protected:
  plJoltSettings m_Settings;
};
