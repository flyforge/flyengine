#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

class plJoltMaterial : public JPH::PhysicsMaterial
{
public:
  plJoltMaterial();
  ~plJoltMaterial();

  plSurfaceResource* m_pSurface = nullptr;

  float m_fRestitution = 0.0f;
  float m_fFriction = 0.2f;
};
