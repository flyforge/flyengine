#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// \brief A curve resource can contain more than one curve, but all of the same type.
struct PL_CORE_DLL plCurve1DResourceDescriptor
{
  plDynamicArray<plCurve1D> m_Curves;

  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);
};

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

/// \brief A resource that stores 1D curves. The curves are stored in the descriptor.
class PL_CORE_DLL plCurve1DResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plCurve1DResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plCurve1DResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plCurve1DResource, plCurve1DResourceDescriptor);

public:
  plCurve1DResource();

  /// \brief Returns all the data that is stored in this resource.
  const plCurve1DResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plCurve1DResourceDescriptor m_Descriptor;
};
