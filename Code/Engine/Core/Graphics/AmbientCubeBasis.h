#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

struct PLASMA_CORE_DLL plAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static plVec3 s_Dirs[NumDirs];
};

template <typename T>
struct plAmbientCube
{
  PLASMA_DECLARE_POD_TYPE();

  plAmbientCube();

  template <typename U>
  plAmbientCube(const plAmbientCube<U>& other);

  template <typename U>
  void operator=(const plAmbientCube<U>& other);

  bool operator==(const plAmbientCube& other) const;
  bool operator!=(const plAmbientCube& other) const;

  void AddSample(const plVec3& vDir, const T& value);

  T Evaluate(const plVec3& vNormal) const;

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);

  T m_Values[plAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
