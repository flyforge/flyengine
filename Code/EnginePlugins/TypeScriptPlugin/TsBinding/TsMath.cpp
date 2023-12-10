#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushVec2(duk_context* pDuk, const plVec2& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Vec2").Succeeded(), ""); // [ global __Vec2 ]
  duk_get_prop_string(duk, -1, "Vec2");                     // [ global __Vec2 Vec2 ]
  duk_push_number(duk, value.x);                            // [ global __Vec2 Vec2 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec2 Vec2 x y ]
  duk_new(duk, 2);                                          // [ global __Vec2 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetVec2(duk_context* pDuk, plInt32 iObjIdx, const plVec2& value)
{
  plDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptBinding::SetVec2Property(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plVec2& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec2(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plVec2 plTypeScriptBinding::GetVec2(duk_context* pDuk, plInt32 iObjIdx, const plVec2& fallback /*= plVec2::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plVec2 res;

  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.x));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.y));
  duk_pop(pDuk);

  return res;
}

plVec2 plTypeScriptBinding::GetVec2Property(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plVec2& fallback /*= plVec2::ZeroVector()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plVec2 res = GetVec2(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushVec3(duk_context* pDuk, const plVec3& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Vec3").Succeeded(), ""); // [ global __Vec3 ]
  duk_get_prop_string(duk, -1, "Vec3");                     // [ global __Vec3 Vec3 ]
  duk_push_number(duk, value.x);                            // [ global __Vec3 Vec3 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec3 Vec3 x y ]
  duk_push_number(duk, value.z);                            // [ global __Vec3 Vec3 x y z ]
  duk_new(duk, 3);                                          // [ global __Vec3 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetVec3(duk_context* pDuk, plInt32 iObjIdx, const plVec3& value)
{
  plDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);
  duk.SetNumberProperty("z", value.z, iObjIdx);
}

void plTypeScriptBinding::SetVec3Property(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plVec3& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec3(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plVec3 plTypeScriptBinding::GetVec3(duk_context* pDuk, plInt32 iObjIdx, const plVec3& fallback /*= plVec3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plVec3 res;

  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.x));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.y));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.z));
  duk_pop(pDuk);

  return res;
}

plVec3 plTypeScriptBinding::GetVec3Property(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plVec3& fallback /*= plVec3::ZeroVector()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plVec3 res = GetVec3(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushMat3(duk_context* pDuk, const plMat3& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Mat3").Succeeded(), ""); // [ global __Mat3 ]
  duk_get_prop_string(duk, -1, "Mat3");                     // [ global __Mat3 Mat3 ]

  float rm[9];
  value.GetAsArray(rm, plMatrixLayout::RowMajor);

  for (plUInt32 i = 0; i < 9; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat3 Mat3 9params ]
  }

  duk_new(duk, 9);     // [ global __Mat3 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetMat3(duk_context* pDuk, plInt32 iObjIdx, const plMat3& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptBinding::SetMat3Property(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plMat3& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat3(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plMat3 plTypeScriptBinding::GetMat3(duk_context* pDuk, plInt32 iObjIdx, const plMat3& fallback /*= plMat3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plMat3 res;

  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0] = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1] = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2] = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3] = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4] = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5] = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6] = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7] = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8] = duk.GetFloatProperty("8", 0.0f);

  duk.PopStack();

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

plMat3 plTypeScriptBinding::GetMat3Property(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plMat3& fallback /*= plMat3::ZeroVector()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plMat3 res = GetMat3(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushMat4(duk_context* pDuk, const plMat4& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Mat4").Succeeded(), ""); // [ global __Mat4 ]
  duk_get_prop_string(duk, -1, "Mat4");                     // [ global __Mat4 Mat4 ]

  float rm[16];
  value.GetAsArray(rm, plMatrixLayout::RowMajor);

  for (plUInt32 i = 0; i < 16; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat4 Mat4 16params ]
  }

  duk_new(duk, 16);    // [ global __Mat4 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetMat4(duk_context* pDuk, plInt32 iObjIdx, const plMat4& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.SetNumberProperty("9", value.m_fElementsCM[9], -1);
  duk.SetNumberProperty("10", value.m_fElementsCM[10], -1);
  duk.SetNumberProperty("11", value.m_fElementsCM[11], -1);
  duk.SetNumberProperty("12", value.m_fElementsCM[12], -1);
  duk.SetNumberProperty("13", value.m_fElementsCM[13], -1);
  duk.SetNumberProperty("14", value.m_fElementsCM[14], -1);
  duk.SetNumberProperty("15", value.m_fElementsCM[15], -1);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptBinding::SetMat4Property(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plMat4& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat4(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plMat4 plTypeScriptBinding::GetMat4(duk_context* pDuk, plInt32 iObjIdx, const plMat4& fallback /*= plMat4::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plMat4 res;

  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0] = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1] = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2] = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3] = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4] = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5] = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6] = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7] = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8] = duk.GetFloatProperty("8", 0.0f);
  res.m_fElementsCM[9] = duk.GetFloatProperty("9", 0.0f);
  res.m_fElementsCM[10] = duk.GetFloatProperty("10", 0.0f);
  res.m_fElementsCM[11] = duk.GetFloatProperty("11", 0.0f);
  res.m_fElementsCM[12] = duk.GetFloatProperty("12", 0.0f);
  res.m_fElementsCM[13] = duk.GetFloatProperty("13", 0.0f);
  res.m_fElementsCM[14] = duk.GetFloatProperty("14", 0.0f);
  res.m_fElementsCM[15] = duk.GetFloatProperty("15", 0.0f);

  duk.PopStack();

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

plMat4 plTypeScriptBinding::GetMat4Property(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plMat4& fallback /*= plMat4::ZeroVector()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plMat4 res = GetMat4(pDuk, -1, fallback);
  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushQuat(duk_context* pDuk, const plQuat& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Quat").Succeeded(), ""); // [ global __Quat ]
  duk_get_prop_string(duk, -1, "Quat");                     // [ global __Quat Quat ]
  duk_push_number(duk, value.x);                          // [ global __Quat Quat x ]
  duk_push_number(duk, value.y);                          // [ global __Quat Quat x y ]
  duk_push_number(duk, value.z);                          // [ global __Quat Quat x y z ]
  duk_push_number(duk, value.w);                            // [ global __Quat Quat x y z w ]
  duk_new(duk, 4);                                          // [ global __Quat result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetQuat(duk_context* pDuk, plInt32 iObjIdx, const plQuat& value)
{
  plDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);
  duk.SetNumberProperty("z", value.z, iObjIdx);
  duk.SetNumberProperty("w", value.w, iObjIdx);
}

void plTypeScriptBinding::SetQuatProperty(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plQuat& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetQuat(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plQuat plTypeScriptBinding::GetQuat(duk_context* pDuk, plInt32 iObjIdx, plQuat fallback /*= plQuat::IdentityQuaternion()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plQuat res;

  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.x));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.y));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.z));
  duk_pop(pDuk);
  PLASMA_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = static_cast<float>(duk_get_number_default(pDuk, -1, fallback.w));
  duk_pop(pDuk);

  return res;
}

plQuat plTypeScriptBinding::GetQuatProperty(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, plQuat fallback /*= plQuat::IdentityQuaternion()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plQuat res = GetQuat(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushColor(duk_context* pDuk, const plColor& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Color").Succeeded(), ""); // [ global __Color ]
  duk_get_prop_string(duk, -1, "Color");                     // [ global __Color Color ]
  duk_push_number(duk, value.r);                             // [ global __Color Color r ]
  duk_push_number(duk, value.g);                             // [ global __Color Color r g ]
  duk_push_number(duk, value.b);                             // [ global __Color Color r g b ]
  duk_push_number(duk, value.a);                             // [ global __Color Color r g b a ]
  duk_new(duk, 4);                                           // [ global __Color result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetColor(duk_context* pDuk, plInt32 iObjIdx, const plColor& value)
{
  plDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("r", value.r, iObjIdx);
  duk.SetNumberProperty("g", value.g, iObjIdx);
  duk.SetNumberProperty("b", value.b, iObjIdx);
  duk.SetNumberProperty("a", value.a, iObjIdx);

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptBinding::SetColorProperty(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plColor& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetColor(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plColor plTypeScriptBinding::GetColor(duk_context* pDuk, plInt32 iObjIdx, const plColor& fallback /*= plColor::White*/)
{
  plDuktapeHelper duk(pDuk);

  plColor res;
  res.r = duk.GetFloatProperty("r", fallback.r, iObjIdx);
  res.g = duk.GetFloatProperty("g", fallback.g, iObjIdx);
  res.b = duk.GetFloatProperty("b", fallback.b, iObjIdx);
  res.a = duk.GetFloatProperty("a", fallback.a, iObjIdx);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

plColor plTypeScriptBinding::GetColorProperty(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plColor& fallback /*= plColor::White*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plColor res = GetColor(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushTransform(duk_context* pDuk, const plTransform& value)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                        // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__Transform").Succeeded(), ""); // [ global __Transform ]
  duk_get_prop_string(duk, -1, "Transform");                     // [ global __Transform Transform ]
  duk_new(duk, 0);                                               // [ global __Transform object ]
  SetVec3Property(pDuk, "position", -1, value.m_vPosition);      // [ global __Transform object ]
  SetQuatProperty(pDuk, "rotation", -1, value.m_qRotation);      // [ global __Transform object ]
  SetVec3Property(pDuk, "scale", -1, value.m_vScale);            // [ global __Transform object ]
  duk_remove(duk, -2);                                           // [ global object ]
  duk_remove(duk, -2);                                           // [ object ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetTransform(duk_context* pDuk, plInt32 iObjIdx, const plTransform& value)
{
  SetVec3Property(pDuk, "position", iObjIdx, value.m_vPosition);
  SetQuatProperty(pDuk, "rotation", iObjIdx, value.m_qRotation);
  SetVec3Property(pDuk, "scale", iObjIdx, value.m_vScale);
}

void plTypeScriptBinding::SetTransformProperty(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plTransform& value)
{
  plDuktapeHelper duk(pDuk);

  PLASMA_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetTransform(pDuk, -1, value);
  duk.PopStack();

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plTransform plTypeScriptBinding::GetTransform(duk_context* pDuk, plInt32 iObjIdx, const plTransform& fallback /*= plTransform::IdentityTransform()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  plTransform res;

  res.m_vPosition = GetVec3Property(pDuk, "position", iObjIdx, fallback.m_vPosition);
  res.m_qRotation = GetQuatProperty(pDuk, "rotation", iObjIdx, fallback.m_qRotation);
  res.m_vScale = GetVec3Property(pDuk, "scale", iObjIdx, fallback.m_vScale);

  return res;
}

plTransform plTypeScriptBinding::GetTransformProperty(
  duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plTransform& fallback /*= plTransform::IdentityTransform()*/)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const plTransform res = GetTransform(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void plTypeScriptBinding::PushVariant(duk_context* pDuk, const plVariant& value)
{
  plDuktapeHelper duk(pDuk);

  switch (value.GetType())
  {
    case plVariant::Type::Angle:
      duk.PushNumber(value.Get<plAngle>().GetRadian());
      break;

    case plVariant::Type::Time:
      duk.PushNumber(value.Get<plTime>().GetSeconds());
      break;

    case plVariant::Type::Bool:
      duk.PushBool(value.Get<bool>());
      break;

    case plVariant::Type::Int8:
    case plVariant::Type::UInt8:
    case plVariant::Type::Int16:
    case plVariant::Type::UInt16:
    case plVariant::Type::Int32:
    case plVariant::Type::UInt32:
    case plVariant::Type::Int64:
    case plVariant::Type::UInt64:
    case plVariant::Type::Float:
    case plVariant::Type::Double:
      duk.PushNumber(value.ConvertTo<double>());
      break;

    case plVariant::Type::Color:
    case plVariant::Type::ColorGamma:
      PushColor(duk, value.ConvertTo<plColor>());
      break;

    case plVariant::Type::Vector2:
      PushVec2(duk, value.Get<plVec2>());
      break;

    case plVariant::Type::Vector3:
      PushVec3(duk, value.Get<plVec3>());
      break;

    case plVariant::Type::Quaternion:
      PushQuat(duk, value.Get<plQuat>());
      break;

    case plVariant::Type::Transform:
      PushTransform(duk, value.Get<plTransform>());
      break;

    case plVariant::Type::String:
      duk.PushString(value.Get<plString>());
      break;

    case plVariant::Type::StringView:
      duk.PushString(value.Get<plStringView>());
      break;

    case plVariant::Type::HashedString:
      duk.PushString(value.Get<plHashedString>());
      break;

    case plVariant::Type::Vector2I:
    {
      const plVec2I32 v = value.Get<plVec2I32>();
      PushVec2(duk, plVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case plVariant::Type::Vector2U:
    {
      const plVec2U32 v = value.Get<plVec2U32>();
      PushVec2(duk, plVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case plVariant::Type::Vector3I:
    {
      const plVec3I32 v = value.Get<plVec3I32>();
      PushVec3(duk, plVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case plVariant::Type::Vector3U:
    {
      const plVec3U32 v = value.Get<plVec3U32>();
      PushVec3(duk, plVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case plVariant::Type::Matrix3:
      PushMat3(duk, value.Get<plMat3>());
      break;

    case plVariant::Type::Matrix4:
      PushMat4(duk, value.Get<plMat4>());
      break;

      // TODO: implement these types
      // case plVariant::Type::Vector4:
      // case plVariant::Type::Vector4I:
      // case plVariant::Type::Vector4U:

    case plVariant::Type::TypedObject:
      duk.PushUndefined();
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      duk.PushUndefined();
      break;
  }

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::SetVariantProperty(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plVariant& value)
{
  plDuktapeHelper duk(pDuk);

  PushVariant(pDuk, value);
  duk.SetCustomProperty(szPropertyName, iObjIdx);

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plVariant plTypeScriptBinding::GetVariant(duk_context* pDuk, plInt32 iObjIdx, const plRTTI* pType)
{
  plDuktapeHelper duk(pDuk);

  if (pType->IsDerivedFrom<plEnumBase>() || pType->IsDerivedFrom<plBitflagsBase>())
  {
    return duk.GetIntValue(iObjIdx);
  }

  switch (pType->GetVariantType())
  {
    case plVariant::Type::Invalid:
    {
      if (pType->GetTypeName() == "plVariant")
      {
        switch (duk_get_type(duk.GetContext(), iObjIdx))
        {
          case DUK_TYPE_BOOLEAN:
            return duk.GetBoolValue(iObjIdx);
          case DUK_TYPE_NUMBER:
            return duk.GetFloatValue(iObjIdx);
          case DUK_TYPE_STRING:
            return duk.GetStringValue(iObjIdx);

          default:
            return plVariant();
        }
      }

      return plVariant();
    }

    case plVariant::Type::Bool:
      return duk.GetBoolValue(iObjIdx);

    case plVariant::Type::Angle:
      return plAngle::MakeFromRadian(duk.GetFloatValue(iObjIdx));

    case plVariant::Type::Time:
      return plTime::Seconds(duk.GetFloatValue(iObjIdx));

    case plVariant::Type::Int8:
    case plVariant::Type::Int16:
    case plVariant::Type::Int32:
    case plVariant::Type::Int64:
      return duk.GetIntValue(iObjIdx);

    case plVariant::Type::UInt8:
    case plVariant::Type::UInt16:
    case plVariant::Type::UInt32:
    case plVariant::Type::UInt64:
      return duk.GetUIntValue(iObjIdx);

    case plVariant::Type::Float:
      return duk.GetFloatValue(iObjIdx);

    case plVariant::Type::Double:
      return duk.GetNumberValue(iObjIdx);

    case plVariant::Type::String:
      return duk.GetStringValue(iObjIdx);

    case plVariant::Type::StringView:
      return plStringView(duk.GetStringValue(iObjIdx));

    case plVariant::Type::HashedString:
    {
      plHashedString sValue;
      sValue.Assign(duk.GetStringValue(iObjIdx));
      return sValue;
    }

    case plVariant::Type::Vector2:
      return plTypeScriptBinding::GetVec2(duk, iObjIdx);

    case plVariant::Type::Vector3:
      return plTypeScriptBinding::GetVec3(duk, iObjIdx);

    case plVariant::Type::Quaternion:
      return plTypeScriptBinding::GetQuat(duk, iObjIdx);

    case plVariant::Type::Transform:
      return plTypeScriptBinding::GetTransform(duk, iObjIdx);

    case plVariant::Type::Color:
      return plTypeScriptBinding::GetColor(duk, iObjIdx);

    case plVariant::Type::ColorGamma:
      return plColorGammaUB(plTypeScriptBinding::GetColor(duk, iObjIdx));

    case plVariant::Type::Vector2I:
    {
      const plVec2 v = plTypeScriptBinding::GetVec2(duk, iObjIdx);
      return plVec2I32(static_cast<plInt32>(v.x), static_cast<plInt32>(v.y));
    }

    case plVariant::Type::Vector3I:
    {
      const plVec3 v = plTypeScriptBinding::GetVec3(duk, iObjIdx);
      return plVec3I32(static_cast<plInt32>(v.x), static_cast<plInt32>(v.y), static_cast<plInt32>(v.z));
    }

    case plVariant::Type::Vector2U:
    {
      const plVec2 v = plTypeScriptBinding::GetVec2(duk, iObjIdx);
      return plVec2U32(static_cast<plUInt32>(v.x), static_cast<plUInt32>(v.y));
    }

    case plVariant::Type::Vector3U:
    {
      const plVec3 v = plTypeScriptBinding::GetVec3(duk, iObjIdx);
      return plVec3U32(static_cast<plUInt32>(v.x), static_cast<plUInt32>(v.y), static_cast<plUInt32>(v.z));
    }

    case plVariant::Type::Matrix3:
      return plTypeScriptBinding::GetMat3(duk, iObjIdx);

    case plVariant::Type::Matrix4:
      return plTypeScriptBinding::GetMat4(duk, iObjIdx);

      // case plVariant::Type::Vector4:
      //  break;
      // case plVariant::Type::Vector4I:
      //  break;
      // case plVariant::Type::Vector4U:
      //  break;

      // case plVariant::Type::Uuid:
      //  break;
      
    case plVariant::Type::TypedObject:
      return plVariant();
    
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return plVariant();
}

plVariant plTypeScriptBinding::GetVariantProperty(duk_context* pDuk, const char* szPropertyName, plInt32 iObjIdx, const plRTTI* pType)
{
  plDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return plVariant();
  }

  const plVariant res = GetVariant(pDuk, -1, pType);
  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}
