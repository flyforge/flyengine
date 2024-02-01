#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <Foundation/Types/Uuid.h>
#  include <Windows.Foundation.numerics.h>

plMat4 plUwpUtils::ConvertMat4(const ABI::Windows::Foundation::Numerics::Matrix4x4& in)
{
  return plMat4(in.M11, in.M21, in.M31, in.M41, in.M12, in.M22, in.M32, in.M42, in.M13, in.M23, in.M33, in.M43, in.M14, in.M24, in.M34, in.M44);
}

plVec3 plUwpUtils::ConvertVec3(const ABI::Windows::Foundation::Numerics::Vector3& in)
{
  return plVec3(in.X, in.Y, in.Z);
}

void plUwpUtils::ConvertVec3(const plVec3& in, ABI::Windows::Foundation::Numerics::Vector3& out)
{
  out.X = in.x;
  out.Y = in.y;
  out.Z = in.z;
}

plQuat plUwpUtils::ConvertQuat(const ABI::Windows::Foundation::Numerics::Quaternion& in)
{
  return plQuat(in.X, in.Y, in.Z, in.W);
}

void plUwpUtils::ConvertQuat(const plQuat& in, ABI::Windows::Foundation::Numerics::Quaternion& out)
{
  out.X = in.x;
  out.Y = in.y;
  out.Z = in.z;
  out.W = in.w;
}

plUuid plUwpUtils::ConvertGuid(const GUID& in)
{
  return *reinterpret_cast<const plUuid*>(&in);
}

void plUwpUtils::ConvertGuid(const plUuid& in, GUID& out)
{
  plMemoryUtils::Copy(reinterpret_cast<plUInt32*>(&out), reinterpret_cast<const plUInt32*>(&in), 4);
}


#endif


