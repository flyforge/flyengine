#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

struct PLASMA_GAMEENGINE_DLL plGrabbableItemGrabPoint
{
  plVec3 m_vLocalPosition;
  plQuat m_qLocalRotation;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plGrabbableItemGrabPoint);

//////////////////////////////////////////////////////////////////////////

using plGrabbableItemComponentManager = plComponentManager<class plGrabbableItemComponent, plBlockStorageType::Compact>;

/// \brief Used to define 'grab points' on an object where a player can pick up and hold the item
///
/// The grabbable item component is typically added to objects with a dynamic physics actor to mark it as an item that can be
/// picked up, and to define the anchor points at which the object can be held.
/// Of course a game can utilize this information without a physical actor and physically holding objects as well.
///
/// Each grab point defines how the object would be oriented when held.
///
/// The component only holds data, it doesn't add any custom behavior. It is the responsibility of other components to use this
/// data in a sensible way.
class PLASMA_GAMEENGINE_DLL plGrabbableItemComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plGrabbableItemComponent, plComponent, plGrabbableItemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plGrabbableItemComponent

public:
  plGrabbableItemComponent();
  ~plGrabbableItemComponent();

  plUInt32 GrabPoints_GetCount() const;                                       // [ property ]
  plGrabbableItemGrabPoint GrabPoints_GetValue(plUInt32 uiIndex) const;       // [ property ]
  void GrabPoints_SetValue(plUInt32 uiIndex, plGrabbableItemGrabPoint value); // [ property ]
  void GrabPoints_Insert(plUInt32 uiIndex, plGrabbableItemGrabPoint value);   // [ property ]
  void GrabPoints_Remove(plUInt32 uiIndex);                                   // [ property ]

  plDynamicArray<plGrabbableItemGrabPoint> m_GrabPoints;
};
