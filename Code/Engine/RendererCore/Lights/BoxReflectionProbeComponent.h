#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class PLASMA_RENDERERCORE_DLL plBoxReflectionProbeComponentManager final : public plComponentManager<class plBoxReflectionProbeComponent, plBlockStorageType::Compact>
{
public:
  plBoxReflectionProbeComponentManager(plWorld* pWorld);
};

/// \brief Box reflection probe component.
///
/// The generated reflection cube map is projected on a box defined by this component's extents. The influence volume can be smaller than the projection which is defined by a scale and shift parameter. Each side of the influence volume has a separate falloff parameter to smoothly blend the probe into others.
class PLASMA_RENDERERCORE_DLL plBoxReflectionProbeComponent : public plReflectionProbeComponentBase
{
  PLASMA_DECLARE_COMPONENT_TYPE(plBoxReflectionProbeComponent, plReflectionProbeComponentBase, plBoxReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plBoxReflectionProbeComponent

public:
  plBoxReflectionProbeComponent();
  ~plBoxReflectionProbeComponent();

  const plVec3& GetExtents() const;       // [ property ]
  void SetExtents(const plVec3& vExtents); // [ property ]

  const plVec3& GetInfluenceScale() const;               // [ property ]
  void SetInfluenceScale(const plVec3& vInfluenceScale); // [ property ]
  const plVec3& GetInfluenceShift() const;               // [ property ]
  void SetInfluenceShift(const plVec3& vInfluenceShift); // [ property ]

  void SetPositiveFalloff(const plVec3& vFalloff);                        // [ property ]
  const plVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; } // [ property ]
  void SetNegativeFalloff(const plVec3& vFalloff);                        // [ property ]
  const plVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; } // [ property ]

  void SetBoxProjection(bool bBoxProjection);                // [ property ]
  bool GetBoxProjection() const { return m_bBoxProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const plAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnTransformChanged(plMsgTransformChanged& msg);

protected:
  plVec3 m_vExtents = plVec3(5.0f);
  plVec3 m_vInfluenceScale = plVec3(1.0f);
  plVec3 m_vInfluenceShift = plVec3(0.0f);
  plVec3 m_vPositiveFalloff = plVec3(0.1f, 0.1f, 0.0f);
  plVec3 m_vNegativeFalloff = plVec3(0.1f, 0.1f, 0.0f);
  bool m_bBoxProjection = true;
};

/// \brief A special visualizer attribute for box reflection probes
class PLASMA_RENDERERCORE_DLL plBoxReflectionProbeVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBoxReflectionProbeVisualizerAttribute, plVisualizerAttribute);

public:
  plBoxReflectionProbeVisualizerAttribute();

  plBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty);

  const plUntrackedString& GetExtentsProperty() const { return m_sProperty1; }
  const plUntrackedString& GetInfluenceScaleProperty() const { return m_sProperty2; }
  const plUntrackedString& GetInfluenceShiftProperty() const { return m_sProperty3; }
};
