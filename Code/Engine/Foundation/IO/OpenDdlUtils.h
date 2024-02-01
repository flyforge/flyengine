#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

class plOpenDdlReader;
class plOpenDdlWriter;
class plOpenDdlReaderElement;

namespace plOpenDdlUtils
{
  /// \brief Converts the data that \a pElement points to to an plColor.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as plColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns PL_FAILURE.
  PL_FOUNDATION_DLL plResult ConvertToColor(const plOpenDdlReaderElement* pElement, plColor& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plColorGammaUB.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as plColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns PL_FAILURE.
  PL_FOUNDATION_DLL plResult ConvertToColorGamma(const plOpenDdlReaderElement* pElement, plColorGammaUB& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plTime.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float or double.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToTime(const plOpenDdlReaderElement* pElement, plTime& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec2.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec2(const plOpenDdlReaderElement* pElement, plVec2& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec3.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec3(const plOpenDdlReaderElement* pElement, plVec3& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec4.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec4(const plOpenDdlReaderElement* pElement, plVec4& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec2I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec2I(const plOpenDdlReaderElement* pElement, plVec2I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec3I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec3I(const plOpenDdlReaderElement* pElement, plVec3I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec4I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec4I(const plOpenDdlReaderElement* pElement, plVec4I32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec2U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec2U(const plOpenDdlReaderElement* pElement, plVec2U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec3U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec3U(const plOpenDdlReaderElement* pElement, plVec3U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plVec4U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToVec4U(const plOpenDdlReaderElement* pElement, plVec4U32& out_vResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plMat3.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 floats.
  /// The elements are expected to be in column-major format. See plMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToMat3(const plOpenDdlReaderElement* pElement, plMat3& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plMat4.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 floats.
  /// The elements are expected to be in column-major format. See plMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToMat4(const plOpenDdlReaderElement* pElement, plMat4& out_mResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plTransform.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 floats.
  /// The first 9 elements are expected to be a mat3 in column-major format. See plMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToTransform(const plOpenDdlReaderElement* pElement, plTransform& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plQuat.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToQuat(const plOpenDdlReaderElement* pElement, plQuat& out_qResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plUuid.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 unsigned_int64.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToUuid(const plOpenDdlReaderElement* pElement, plUuid& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plAngle.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in radians.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToAngle(const plOpenDdlReaderElement* pElement, plAngle& out_result); // [tested]

  /// \brief Converts the data that \a pElement points to to an plHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 string.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToHashedString(const plOpenDdlReaderElement* pElement, plHashedString& out_sResult); // [tested]

  /// \brief Converts the data that \a pElement points to to an plTempHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  PL_FOUNDATION_DLL plResult ConvertToTempHashedString(const plOpenDdlReaderElement* pElement, plTempHashedString& out_sResult); // [tested]

  /// \brief Uses the elements custom type name to infer which type the object holds and reads it into the plVariant.
  ///
  /// Depending on the custom type name, one of the other ConvertToXY functions is called and the respective conditions to the data format apply.
  /// Supported type names are: "Color", "ColorGamma", "Time", "Vec2", "Vec3", "Vec4", "Mat3", "Mat4", "Transform", "Quat", "Uuid", "Angle", "HashedString", "TempHashedString"
  /// Type names are case sensitive.
  PL_FOUNDATION_DLL plResult ConvertToVariant(const plOpenDdlReaderElement* pElement, plVariant& out_result); // [tested]

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  /// \brief Writes an plColor to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreColor(plOpenDdlWriter& ref_writer, const plColor& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plColorGammaUB to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreColorGamma(plOpenDdlWriter& ref_writer, const plColorGammaUB& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plTime to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreTime(plOpenDdlWriter& ref_writer, const plTime& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec2 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec2(plOpenDdlWriter& ref_writer, const plVec2& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec3 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec3(plOpenDdlWriter& ref_writer, const plVec3& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec4 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec4(plOpenDdlWriter& ref_writer, const plVec4& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec2 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec2I(plOpenDdlWriter& ref_writer, const plVec2I32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec3 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec3I(plOpenDdlWriter& ref_writer, const plVec3I32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec4 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec4I(plOpenDdlWriter& ref_writer, const plVec4I32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec2 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec2U(plOpenDdlWriter& ref_writer, const plVec2U32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec3 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec3U(plOpenDdlWriter& ref_writer, const plVec3U32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVec4 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVec4U(plOpenDdlWriter& ref_writer, const plVec4U32& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plMat3 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreMat3(plOpenDdlWriter& ref_writer, const plMat3& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plMat4 to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreMat4(plOpenDdlWriter& ref_writer, const plMat4& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plTransform to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreTransform(plOpenDdlWriter& ref_writer, const plTransform& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plQuat to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreQuat(plOpenDdlWriter& ref_writer, const plQuat& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plUuid to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreUuid(plOpenDdlWriter& ref_writer, const plUuid& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plAngle to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreAngle(plOpenDdlWriter& ref_writer, const plAngle& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plHashedString to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreHashedString(plOpenDdlWriter& ref_writer, const plHashedString& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plTempHashedString to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreTempHashedString(plOpenDdlWriter& ref_writer, const plTempHashedString& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an plVariant to DDL such that the type can be reconstructed.
  PL_FOUNDATION_DLL void StoreVariant(plOpenDdlWriter& ref_writer, const plVariant& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single string and an optional name.
  PL_FOUNDATION_DLL void StoreString(plOpenDdlWriter& ref_writer, const plStringView& value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreBool(plOpenDdlWriter& ref_writer, bool value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreFloat(plOpenDdlWriter& ref_writer, float value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreDouble(plOpenDdlWriter& ref_writer, double value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreInt8(plOpenDdlWriter& ref_writer, plInt8 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreInt16(plOpenDdlWriter& ref_writer, plInt16 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreInt32(plOpenDdlWriter& ref_writer, plInt32 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreInt64(plOpenDdlWriter& ref_writer, plInt64 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreUInt8(plOpenDdlWriter& ref_writer, plUInt8 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreUInt16(plOpenDdlWriter& ref_writer, plUInt16 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreUInt32(plOpenDdlWriter& ref_writer, plUInt32 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes a primitives list with a single value and an optional name.
  PL_FOUNDATION_DLL void StoreUInt64(plOpenDdlWriter& ref_writer, plUInt64 value, plStringView sName = {}, bool bGlobalName = false); // [tested]

  /// \brief Writes an invalid variant and an optional name.
  PL_FOUNDATION_DLL void StoreInvalid(plOpenDdlWriter& ref_writer, plStringView sName = {}, bool bGlobalName = false);
} // namespace plOpenDdlUtils
