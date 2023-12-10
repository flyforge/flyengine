#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/Curve1D.h>

/// \brief A curve resource can contain more than one curve, but all of the same type.
struct PLASMA_CORE_DLL plCurve1DResourceDescriptor
{
  plDynamicArray<plCurve1D> m_Curves;

  void Save(plStreamWriter& stream) const;
  void Load(plStreamReader& stream);
};

using plCurve1DResourceHandle = plTypedResourceHandle<class plCurve1DResource>;

/// \brief A resource that stores 1D curves. The curves are stored in the descriptor.
class PLASMA_CORE_DLL plCurve1DResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCurve1DResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plCurve1DResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plCurve1DResource, plCurve1DResourceDescriptor);

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
