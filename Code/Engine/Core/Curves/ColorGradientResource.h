#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>

struct PLASMA_CORE_DLL plColorGradientResourceDescriptor
{
  plColorGradient m_Gradient;

  void Save(plStreamWriter& stream) const;
  void Load(plStreamReader& stream);
};

using plColorGradientResourceHandle = plTypedResourceHandle<class plColorGradientResource>;

/// \brief A resource that stores a single color gradient. The data is stored in the descriptor.
class PLASMA_CORE_DLL plColorGradientResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorGradientResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plColorGradientResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plColorGradientResource, plColorGradientResourceDescriptor);

public:
  plColorGradientResource();

  /// \brief Returns all the data that is stored in this resource.
  const plColorGradientResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  inline plColor Evaluate(double x) const
  {
    plColor result;
    m_Descriptor.m_Gradient.Evaluate(x, result);
    return result;
  }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plColorGradientResourceDescriptor m_Descriptor;
};
