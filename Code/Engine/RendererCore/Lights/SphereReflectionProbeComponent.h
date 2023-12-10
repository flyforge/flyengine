#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class PLASMA_RENDERERCORE_DLL plSphereReflectionProbeComponentManager final : public plComponentManager<class plSphereReflectionProbeComponent, plBlockStorageType::Compact>
{
public:
  plSphereReflectionProbeComponentManager(plWorld* pWorld);
};

//////////////////////////////////////////////////////////////////////////
// plSphereReflectionProbeComponent

/// \brief Sphere reflection probe component.
///
/// The generated reflection cube map is is projected to infinity. So parallax correction takes place.
class PLASMA_RENDERERCORE_DLL plSphereReflectionProbeComponent : public plReflectionProbeComponentBase
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSphereReflectionProbeComponent, plReflectionProbeComponentBase, plSphereReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plSphereReflectionProbeComponent

public:
  plSphereReflectionProbeComponent();
  ~plSphereReflectionProbeComponent();

  void SetRadius(float fRadius); // [ property ]
  float GetRadius() const;       // [ property ]

  void SetFalloff(float fFalloff);                // [ property ]
  float GetFalloff() const { return m_fFalloff; } // [ property ]

  void SetSphereProjection(bool bSphereProjection);                // [ property ]
  bool GetSphereProjection() const { return m_bSphereProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const plAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnTransformChanged(plMsgTransformChanged& msg);
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.1f;
  bool m_bSphereProjection = true;
};
