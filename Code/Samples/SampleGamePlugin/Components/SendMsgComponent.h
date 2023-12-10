#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct plMsgComponentInternalTrigger;

// This component manager does literally nothing, meaning the managed components do not need to be update, at all
// BEGIN-DOCS-CODE-SNIPPET: component-manager-trivial
using SendMsgComponentManager = plComponentManager<class SendMsgComponent, plBlockStorageType::Compact>;
// END-DOCS-CODE-SNIPPET

class PLASMA_SAMPLEGAMEPLUGIN_DLL SendMsgComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(SendMsgComponent, plComponent, SendMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SendMsgComponent

public:
  SendMsgComponent();
  ~SendMsgComponent();

private:
  plDynamicArray<plString> m_TextArray; // [ property ]

  void OnSendText(plMsgComponentInternalTrigger& msg); // [ msg handler ]

  plUInt32 m_uiNextString = 0;
};
