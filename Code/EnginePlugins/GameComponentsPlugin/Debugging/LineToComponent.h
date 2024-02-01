#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

class plLineToComponentManager : public plComponentManager<class plLineToComponent, plBlockStorageType::FreeList>
{
  using SUPER = plComponentManager<class plLineToComponent, plBlockStorageType::FreeList>;

public:
  plLineToComponentManager(plWorld* pWorld);

protected:
  void Initialize() override;
  void Update(const plWorldModule::UpdateContext& context);
};

/// \brief Draws a line from its own position to the target object position
class PL_GAMECOMPONENTS_DLL plLineToComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plLineToComponent, plComponent, plLineToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plLineToComponent

public:
  plLineToComponent();
  ~plLineToComponent();

  const char* GetLineToTargetGuid() const;            // [ property ]
  void SetLineToTargetGuid(const char* szTargetGuid); // [ property ]

  void SetLineToTarget(const plGameObjectHandle& hTargetObject);                // [ property ]
  const plGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; } // [ property ]

  plColor m_LineColor; // [ property ]

protected:
  void Update();

  plGameObjectHandle m_hTargetObject;
};
