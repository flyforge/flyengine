#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using plMarkerComponentManager = plComponentManager<class plMarkerComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plMarkerComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plMarkerComponent, plComponent, plMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plMarkerComponent

public:
  plMarkerComponent();
  ~plMarkerComponent();

  void SetMarkerType(const char* szType); // [ property ]
  const char* GetMarkerType() const;      // [ property ]

  void SetRadius(float radius); // [ property ]
  float GetRadius() const;      // [ property ]

protected:
  void OnMsgUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const; // [ msg handler ]
  void UpdateMarker();

  float m_fRadius = 0.1f;       // [ property ]
  plHashedString m_sMarkerType; // [ property ]

  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;
};
