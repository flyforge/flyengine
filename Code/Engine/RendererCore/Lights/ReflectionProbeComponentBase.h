#pragma once

#include <Core/World/Component.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;
struct plMsgTransformChanged;
class plAbstractObjectNode;

/// \brief Base class for all reflection probes.
class PL_RENDERERCORE_DLL plReflectionProbeComponentBase : public plComponent
{
  PL_ADD_DYNAMIC_REFLECTION(plReflectionProbeComponentBase, plComponent);

public:
  plReflectionProbeComponentBase();
  ~plReflectionProbeComponentBase();

  void SetReflectionProbeMode(plEnum<plReflectionProbeMode> mode); // [ property ]
  plEnum<plReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  const plTagSet& GetIncludeTags() const;   // [ property ]
  void InsertIncludeTag(const char* szTag); // [ property ]
  void RemoveIncludeTag(const char* szTag); // [ property ]

  const plTagSet& GetExcludeTags() const;   // [ property ]
  void InsertExcludeTag(const char* szTag); // [ property ]
  void RemoveExcludeTag(const char* szTag); // [ property ]

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void SetFarPlane(float fFarPlane);                       // [ property ]

  const plVec3& GetCaptureOffset() const { return m_Desc.m_vCaptureOffset; } // [ property ]
  void SetCaptureOffset(const plVec3& vOffset);                              // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  float ComputePriority(plMsgExtractRenderData& msg, plReflectionProbeRenderData* pRenderData, float fVolume, const plVec3& vScale) const;

protected:
  plReflectionProbeDesc m_Desc;

  plReflectionProbeId m_Id;
  // Set to true if a change was made that requires recomputing the cube map.
  mutable bool m_bStatesDirty = true;
};
