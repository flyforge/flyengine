#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgExtractRenderData;

typedef plComponentManager<class plDebugTextComponent, plBlockStorageType::Compact> plDebugTextComponentManager;

/// \brief This component prints debug text at the owner object's position.
class PLASMA_RENDERERCORE_DLL plDebugTextComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plDebugTextComponent, plComponent, plDebugTextComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent
public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plDebugTextComponent
public:
  plDebugTextComponent();
  ~plDebugTextComponent();

  plString m_sText;       // [ property ]
  plColorGammaUB m_Color; // [ property ]

  float m_fValue0; // [ property ]
  float m_fValue1; // [ property ]
  float m_fValue2; // [ property ]
  float m_fValue3; // [ property ]

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const; // [ msg handler ]
};
