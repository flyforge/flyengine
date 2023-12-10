#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct plMsgSetText;
struct plMsgSetColor;

// BEGIN-DOCS-CODE-SNIPPET: component-manager-simple
using DisplayMsgComponentManager = plComponentManagerSimple<class DisplayMsgComponent, plComponentUpdateType::WhenSimulating, plBlockStorageType::FreeList>;
// END-DOCS-CODE-SNIPPET

class PLASMA_SAMPLEGAMEPLUGIN_DLL DisplayMsgComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(DisplayMsgComponent, plComponent, DisplayMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DisplayMsgComponent

public:
  DisplayMsgComponent();
  ~DisplayMsgComponent();

private:
  void Update();

  void OnSetText(plMsgSetText& msg);   // [ msg handler ]
  void OnSetColor(plMsgSetColor& msg); // [ msg handler ]

  plString m_sCurrentText;
  plColor m_TextColor = plColor::Yellow;
};
