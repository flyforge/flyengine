#include <ModelImporter2/ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace plModelImporter2
{
  plColor ConvertAssimpType(const aiColor4D& value, bool invert /*= false*/)
  {
    if (invert)
      return plColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return plColor(value.r, value.g, value.b, value.a);
  }

  plColor ConvertAssimpType(const aiColor3D& value, bool invert /*= false*/)
  {
    if (invert)
      return plColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return plColor(value.r, value.g, value.b);
  }

  plMat4 ConvertAssimpType(const aiMatrix4x4& value, bool dummy /*= false*/)
  {
    PLASMA_ASSERT_DEBUG(!dummy, "not implemented");

    plMat4 mTransformation;
    mTransformation.SetFromArray(&value.a1, plMatrixLayout::RowMajor);
    return mTransformation;
  }

  plVec3 ConvertAssimpType(const aiVector3D& value, bool dummy /*= false*/)
  {
    PLASMA_ASSERT_DEBUG(!dummy, "not implemented");

    return plVec3(value.x, value.y, value.z);
  }

  plQuat ConvertAssimpType(const aiQuaternion& value, bool dummy /*= false*/)
  {
    PLASMA_ASSERT_DEBUG(!dummy, "not implemented");

    return plQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool dummy /*= false*/)
  {
    PLASMA_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool dummy /*= false*/)
  {
    PLASMA_ASSERT_DEBUG(!dummy, "not implemented");

    return value;
  }

} // namespace plModelImporter2
