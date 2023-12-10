#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Component.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct plMsgExtractGeometry;
struct plMsgUpdateLocalBounds;
class plJoltUserData;
class plJoltMaterial;

namespace JPH
{
  class Shape;
}

struct plJoltSubShape
{
  JPH::Shape* m_pShape = nullptr;
  plTransform m_Transform = plTransform::MakeIdentity();
};

class PLASMA_JOLTPLUGIN_DLL plJoltShapeComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltShapeComponent, plComponent);


  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltShapeComponent

public:
  plJoltShapeComponent();
  ~plJoltShapeComponent();

  virtual void ExtractGeometry(plMsgExtractGeometry& ref_msg) const {}

protected:
  friend class plJoltActorComponent;
  virtual void CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial) = 0;

  const plJoltUserData* GetUserData();
  plUInt32 GetUserDataIndex();

  plUInt32 m_uiUserDataIndex = plInvalidIndex;
};
