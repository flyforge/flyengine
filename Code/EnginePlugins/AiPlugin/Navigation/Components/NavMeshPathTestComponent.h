#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using plNavMeshPathTestComponentManager = plComponentManagerSimple<class plAiNavMeshPathTestComponent, plComponentUpdateType::WhenSimulating>;

class PLASMA_AIPLUGIN_DLL plAiNavMeshPathTestComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAiNavMeshPathTestComponent, plComponent, plNavMeshPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  plAiNavMeshPathTestComponent

public:
  plAiNavMeshPathTestComponent();
  ~plAiNavMeshPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(plGameObjectHandle hObject);

  bool m_bVisualizePathCorridor = true;
  bool m_bVisualizePathLine = true;
  bool m_bVisualizePathState = true;
  plHashedString m_sNavmeshConfig;
  plHashedString m_sPathSearchConfig;

protected:
  void Update();

  plGameObjectHandle m_hPathEnd;
  plAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
