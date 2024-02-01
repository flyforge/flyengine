#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgTransformChanged;
struct plMsgUpdateLocalBounds;
struct plMsgExtractOccluderData;

class PL_RENDERERCORE_DLL plOccluderComponentManager final : public plComponentManager<class plOccluderComponent, plBlockStorageType::FreeList>
{
public:
  plOccluderComponentManager(plWorld* pWorld);
};

/// \brief Adds invisible geometry to a scene that is used for occlusion culling.
///
/// The component adds a box occluder to the scene. The renderer uses this geometry
/// to cull other objects which are behind occluder geometry. Use occluder components to optimize levels.
/// Make the shapes conservative, meaning that they shouldn't be bigger than the actual shapes, otherwise
/// they may incorrectly occlude other objects and lead to incorrectly culled objects.
///
/// The plGreyBoxComponent can also create occluder geometry in different shapes.
///
/// Contrary to plGreyBoxComponent, occluder components can be moved around dynamically and thus can be attached to
/// doors and other objects that may dynamically change the visible areas of a level.
class PL_RENDERERCORE_DLL plOccluderComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plOccluderComponent, plComponent, plOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plOccluderComponent

public:
  plOccluderComponent();
  ~plOccluderComponent();

  /// \brief Sets the size of the box occluder.
  void SetExtents(const plVec3& vExtents);                // [ property ]
  const plVec3& GetExtents() const { return m_vExtents; } // [ property ]

private:
  plVec3 m_vExtents = plVec3(5.0f);

  mutable plSharedPtr<const plRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const;
};
