#pragma once

#include <JoltPlugin/Actors/JoltActorComponent.h>

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltQueryShapeActorComponentManager : public plComponentManager<class plJoltQueryShapeActorComponent, plBlockStorageType::FreeList>
{
public:
  plJoltQueryShapeActorComponentManager(plWorld* pWorld);
  ~plJoltQueryShapeActorComponentManager();

private:
  friend class plJoltWorldModule;
  friend class plJoltQueryShapeActorComponent;

  void UpdateMovingQueryShapes();

  plDynamicArray<plJoltQueryShapeActorComponent*> m_MovingQueryShapes;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A physics actor that can be moved procedurally (like a kinematic actor) but that doesn't affect rigid bodies.
///
/// It passes right through dynamic actors. However, you can detect it via raycasts or shape casts.
/// This is useful to represent detail shapes (like the collision shapes of animated meshes) that should be pickable,
/// but that shouldn't interact with the world otherwise.
/// They are more lightweight at runtime than full kinematic dynamic actors.
class PLASMA_JOLTPLUGIN_DLL plJoltQueryShapeActorComponent : public plJoltActorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltQueryShapeActorComponent, plJoltActorComponent, plJoltQueryShapeActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltQueryShapeActorComponent
public:
  plJoltQueryShapeActorComponent();
  ~plJoltQueryShapeActorComponent();

  void SetSurfaceFile(const char* szFile); // [ property ]
  const char* GetSurfaceFile() const;      // [ property ]

  plSurfaceResourceHandle m_hSurface; // [ property ]

protected:
  const plJoltMaterial* GetJoltMaterial() const;
};
