#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

class plGeometry;

class PL_RENDERERCORE_DLL plRasterizerObject : public plRefCounted
{
  PL_DISALLOW_COPY_AND_ASSIGN(plRasterizerObject);

public:
  plRasterizerObject();
  ~plRasterizerObject();

  /// \brief If an object with the given name has been created before, it is returned, otherwise nullptr is returned.
  ///
  /// Use this to quickly query for an existing object. Call CreateMesh() in case the object doesn't exist yet.
  static plSharedPtr<const plRasterizerObject> GetObject(plStringView sUniqueName);

  /// \brief Creates a box object with the specified dimensions. If such a box was created before, the same pointer is returned.
  static plSharedPtr<const plRasterizerObject> CreateBox(const plVec3& vFullExtents);

  /// \brief Creates an object with the given geometry. If an object with the same name was created before, that pointer is returned instead.
  ///
  /// It is assumed that the same name will only be used for identical geometry.
  static plSharedPtr<const plRasterizerObject> CreateMesh(plStringView sUniqueName, const plGeometry& geometry);

private:
  void CreateMesh(const plGeometry& geometry);

  friend class plRasterizerView;
  Occluder m_Occluder;

  static plMutex s_Mutex;
  static plMap<plString, plSharedPtr<plRasterizerObject>> s_Objects;
};
