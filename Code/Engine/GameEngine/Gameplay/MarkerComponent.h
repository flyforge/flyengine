#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using plMarkerComponentManager = plComponentManager<class plMarkerComponent, plBlockStorageType::Compact>;

/// \brief This component is used to markup objects and locations with gameplay relevant semantical information.
///
/// Objects with marker components can be found through the plSpatialSystem, making it easy to find all marked objects
/// in a certain area.
///
/// Markers use different spatial categories (\see plSpatialData::RegisterCategory()) making it efficient to search
/// only for very specific objects.
///
/// Markers can be used for all sorts of gameplay functionality, usually for AI systems to be able to detect which objects
/// they can interact with.
class PL_GAMEENGINE_DLL plMarkerComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plMarkerComponent, plComponent, plMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plMarkerComponent

public:
  plMarkerComponent();
  ~plMarkerComponent();

  /// \brief The marker type is passed into plSpatialData::RegisterCategory() so that these markers can be found through the plSpatialSystem.
  void SetMarkerType(const char* szType); // [ property ]
  const char* GetMarkerType() const;      // [ property ]

  /// \brief The size of the marker.
  ///
  /// Often this can be very small to just mark a point, but it may be larger to represent the size of the marked object.
  void SetRadius(float fRadius); // [ property ]
  float GetRadius() const;       // [ property ]

protected:
  void OnMsgUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const; // [ msg handler ]
  void UpdateMarker();

  float m_fRadius = 0.1f;       // [ property ]
  plHashedString m_sMarkerType; // [ property ]

  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;
};
