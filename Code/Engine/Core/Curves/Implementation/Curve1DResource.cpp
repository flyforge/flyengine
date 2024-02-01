#include <Core/CorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Curves/Curve1DResource.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurve1DResource, 1, plRTTIDefaultAllocator<plCurve1DResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plCurve1DResource);

plCurve1DResource::plCurve1DResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plCurve1DResource, plCurve1DResourceDescriptor)
{
  m_Descriptor = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plCurve1DResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  m_Descriptor.m_Curves.Clear();

  return res;
}

plResourceLoadDesc plCurve1DResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plCurve1DResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plCurve1DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<plUInt32>(m_Descriptor.m_Curves.GetHeapMemoryUsage()) + static_cast<plUInt32>(sizeof(m_Descriptor));

  for (const auto& curve : m_Descriptor.m_Curves)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += curve.GetHeapMemoryUsage();
  }
}

void plCurve1DResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  const plUInt8 uiCurves = static_cast<plUInt8>(m_Curves.GetCount());
  inout_stream << uiCurves;

  for (plUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Save(inout_stream);
  }
}

void plCurve1DResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  plUInt8 uiCurves = 0;
  inout_stream >> uiCurves;

  m_Curves.SetCount(uiCurves);

  for (plUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Load(inout_stream);

    /// \todo We can do this on load, or somehow ensure this is always already correctly saved
    m_Curves[i].SortControlPoints();
    m_Curves[i].CreateLinearApproximation();
  }
}



PL_STATICLINK_FILE(Core, Core_Curves_Implementation_Curve1DResource);
