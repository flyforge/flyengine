#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/RendererCoreDLL.h>

struct PL_RENDERERCORE_DLL plBakingSettings
{
  plVec3 m_vProbeSpacing = plVec3(4);
  plUInt32 m_uiNumSamplesPerProbe = 128;
  float m_fMaxRayDistance = 1000.0f;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RENDERERCORE_DLL, plBakingSettings);

class plWorld;

class plBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual plResult RenderDebugView(const plWorld& world, const plMat4& mInverseViewProjection, plUInt32 uiWidth, plUInt32 uiHeight, plDynamicArray<plColorGammaUB>& out_pixels, plProgress& ref_progress) const = 0;
};
