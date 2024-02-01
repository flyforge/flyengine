#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plReflectionProbeMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plReflectionProbeMode);

/// \brief Describes how a cube map should be generated.
struct PL_RENDERERCORE_DLL plReflectionProbeDesc
{
  plUuid m_uniqueID;

  plTagSet m_IncludeTags;
  plTagSet m_ExcludeTags;

  plEnum<plReflectionProbeMode> m_Mode;

  bool m_bShowDebugInfo = false;
  bool m_bShowMipMaps = false;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;
  float m_fNearPlane = 0.0f;
  float m_fFarPlane = 100.0f;
  plVec3 m_vCaptureOffset = plVec3::MakeZero();
};

using plReflectionProbeId = plGenericId<24, 8>;

template <>
struct plHashHelper<plReflectionProbeId>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(plReflectionProbeId value) { return plHashHelper<plUInt32>::Hash(value.m_Data); }

  PL_ALWAYS_INLINE static bool Equal(plReflectionProbeId a, plReflectionProbeId b) { return a == b; }
};

/// \brief Render data for a reflection probe.
class PL_RENDERERCORE_DLL plReflectionProbeRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plReflectionProbeRenderData, plRenderData);

public:
  plReflectionProbeRenderData()
  {
    m_Id.Invalidate();
    m_vHalfExtents.SetZero();
  }

  plReflectionProbeId m_Id;
  plUInt32 m_uiIndex = 0;
  plVec3 m_vProbePosition; ///< Probe position in world space.
  plVec3 m_vHalfExtents;
  plVec3 m_vPositiveFalloff;
  plVec3 m_vNegativeFalloff;
  plVec3 m_vInfluenceScale;
  plVec3 m_vInfluenceShift;
};

/// \brief A unique reference to a reflection probe.
struct plReflectionProbeRef
{
  bool operator==(const plReflectionProbeRef& b) const
  {
    return m_Id == b.m_Id && m_uiWorldIndex == b.m_uiWorldIndex;
  }

  plUInt32 m_uiWorldIndex = 0;
  plReflectionProbeId m_Id;
};
PL_CHECK_AT_COMPILETIME(sizeof(plReflectionProbeRef) == 8);

template <>
struct plHashHelper<plReflectionProbeRef>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(plReflectionProbeRef value) { return plHashHelper<plUInt64>::Hash(reinterpret_cast<plUInt64&>(value)); }

  PL_ALWAYS_INLINE static bool Equal(plReflectionProbeRef a, plReflectionProbeRef b) { return a.m_Id == b.m_Id && a.m_uiWorldIndex == b.m_uiWorldIndex; }
};

/// \brief Flags that describe a reflection probe.
struct plProbeFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    SkyLight = PL_BIT(0),
    HasCustomCubeMap = PL_BIT(1),
    Sphere = PL_BIT(2),
    Box = PL_BIT(3),
    Dynamic = PL_BIT(4),
    Default = 0
  };

  struct Bits
  {
    StorageType SkyLight : 1;
    StorageType HasCustomCubeMap : 1;
    StorageType Sphere : 1;
    StorageType Box : 1;
    StorageType Dynamic : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plProbeFlags);

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plProbeFlags);
