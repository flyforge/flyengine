#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/Variant.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// this file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations
#pragma optimize("", off)

class Blubb : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(Blubb, plReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(Blubb, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("u", u),
    PLASMA_MEMBER_PROPERTY("v", v),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
void TestVariant(plVariant& v, plVariantType::Enum type)
{
  PLASMA_TEST_BOOL(v.IsValid());
  PLASMA_TEST_BOOL(v.GetType() == type);
  PLASMA_TEST_BOOL(v.CanConvertTo<T>());
  PLASMA_TEST_BOOL(v.IsA<T>());
  PLASMA_TEST_BOOL(v.GetReflectedType() == plGetStaticRTTI<T>());

  plTypedPointer ptr = v.GetWriteAccess();
  PLASMA_TEST_BOOL(ptr.m_pObject == &v.Get<T>());
  PLASMA_TEST_BOOL(ptr.m_pObject == &v.GetWritable<T>());
  PLASMA_TEST_BOOL(ptr.m_pType == plGetStaticRTTI<T>());

  PLASMA_TEST_BOOL(ptr.m_pObject == v.GetData());

  plVariant vCopy = v;
  plTypedPointer ptr2 = vCopy.GetWriteAccess();
  PLASMA_TEST_BOOL(ptr2.m_pObject == &vCopy.Get<T>());
  PLASMA_TEST_BOOL(ptr2.m_pObject == &vCopy.GetWritable<T>());

  PLASMA_TEST_BOOL(ptr2.m_pObject != ptr.m_pObject);
  PLASMA_TEST_BOOL(ptr2.m_pType == plGetStaticRTTI<T>());

  PLASMA_TEST_BOOL(v.Get<T>() == vCopy.Get<T>());

  PLASMA_TEST_BOOL(v.ComputeHash(0) != 0);
}

template <typename T>
inline void TestIntegerVariant(plVariant::Type::Enum type)
{
  plVariant b((T)23);
  TestVariant<T>(b, type);

  PLASMA_TEST_BOOL(b.Get<T>() == 23);

  PLASMA_TEST_BOOL(b == plVariant(23));
  PLASMA_TEST_BOOL(b != plVariant(11));
  PLASMA_TEST_BOOL(b == plVariant((T)23));
  PLASMA_TEST_BOOL(b != plVariant((T)11));

  PLASMA_TEST_BOOL(b == 23);
  PLASMA_TEST_BOOL(b != 24);
  PLASMA_TEST_BOOL(b == (T)23);
  PLASMA_TEST_BOOL(b != (T)24);

  b = (T)17;
  PLASMA_TEST_BOOL(b == (T)17);

  b = plVariant((T)19);
  PLASMA_TEST_BOOL(b == (T)19);

  PLASMA_TEST_BOOL(b.IsNumber());
  PLASMA_TEST_BOOL(b.IsFloatingPoint() == false);
  PLASMA_TEST_BOOL(!b.IsString());
}

inline void TestNumberCanConvertTo(const plVariant& v)
{
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Invalid) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Bool));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int8));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt8));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int16));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt16));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int32));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt32));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int64));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt64));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Float));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Double));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Color) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2I) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3I) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4I) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Quaternion) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix3) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix4) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Transform) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::String));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::StringView) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::DataBuffer) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Time) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Uuid) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Angle) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::ColorGamma) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::HashedString));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TempHashedString));
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantArray) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantDictionary) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedPointer) == false);
  PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedObject) == false);

  plResult conversionResult = PLASMA_FAILURE;
  PLASMA_TEST_BOOL(v.ConvertTo<bool>(&conversionResult) == true);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plInt8>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plUInt8>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plInt16>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plUInt16>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plInt32>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plUInt32>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plInt64>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plUInt64>(&conversionResult) == 3);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<float>(&conversionResult) == 3.0f);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<double>(&conversionResult) == 3.0);
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plString>(&conversionResult) == "3");
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>(&conversionResult) == plMakeHashedString("3"));
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>(&conversionResult) == plTempHashedString("3"));
  PLASMA_TEST_BOOL(conversionResult.Succeeded());

  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Bool).Get<bool>() == true);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int8).Get<plInt8>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt8).Get<plUInt8>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int16).Get<plInt16>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt16).Get<plUInt16>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int32).Get<plInt32>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt32).Get<plUInt32>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int64).Get<plInt64>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt64).Get<plUInt64>() == 3);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Float).Get<float>() == 3.0f);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Double).Get<double>() == 3.0);
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "3");
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("3"));
  PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("3"));
}

inline void TestCanOnlyConvertToID(const plVariant& v, plVariant::Type::Enum type)
{
  for (int iType = plVariant::Type::FirstStandardType; iType < plVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == plVariant::Type::LastStandardType)
      iType = plVariant::Type::FirstExtendedType;

    if (iType == type)
    {
      PLASMA_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      PLASMA_TEST_BOOL(v.CanConvertTo((plVariant::Type::Enum)iType) == false);
    }
  }
}

inline void TestCanOnlyConvertToStringAndID(const plVariant& v, plVariant::Type::Enum type, plVariant::Type::Enum type2 = plVariant::Type::Invalid,
  plVariant::Type::Enum type3 = plVariant::Type::Invalid)
{
  if (type2 == plVariant::Type::Invalid)
    type2 = type;

  for (int iType = plVariant::Type::FirstStandardType; iType < plVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == plVariant::Type::LastStandardType)
      iType = plVariant::Type::FirstExtendedType;

    if (iType == plVariant::Type::String || iType == plVariant::Type::HashedString || iType == plVariant::Type::TempHashedString)
    {
      PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::String));
    }
    else if (iType == type || iType == type2 || iType == type3)
    {
      PLASMA_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      PLASMA_TEST_BOOL(v.CanConvertTo((plVariant::Type::Enum)iType) == false);
    }
  }
}

PLASMA_CREATE_SIMPLE_TEST(Basics, Variant)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Invalid")
  {
    plVariant b;
    PLASMA_TEST_BOOL(b.GetType() == plVariant::Type::Invalid);
    PLASMA_TEST_BOOL(b == plVariant());
    PLASMA_TEST_BOOL(b != plVariant(0));
    PLASMA_TEST_BOOL(!b.IsValid());
    PLASMA_TEST_BOOL(!b[0].IsValid());
    PLASMA_TEST_BOOL(!b["x"].IsValid());
    PLASMA_TEST_BOOL(b.GetReflectedType() == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "bool")
  {
    plVariant b(true);
    TestVariant<bool>(b, plVariantType::Bool);

    PLASMA_TEST_BOOL(b.Get<bool>() == true);

    PLASMA_TEST_BOOL(b == plVariant(true));
    PLASMA_TEST_BOOL(b != plVariant(false));

    PLASMA_TEST_BOOL(b == true);
    PLASMA_TEST_BOOL(b != false);

    b = false;
    PLASMA_TEST_BOOL(b == false);

    b = plVariant(true);
    PLASMA_TEST_BOOL(b == true);
    PLASMA_TEST_BOOL(!b[0].IsValid());

    PLASMA_TEST_BOOL(b.IsNumber());
    PLASMA_TEST_BOOL(!b.IsString());
    PLASMA_TEST_BOOL(b.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plInt8")
  {
    TestIntegerVariant<plInt8>(plVariant::Type::Int8);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plUInt8")
  {
    TestIntegerVariant<plUInt8>(plVariant::Type::UInt8);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plInt16")
  {
    TestIntegerVariant<plInt16>(plVariant::Type::Int16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plUInt16")
  {
    TestIntegerVariant<plUInt16>(plVariant::Type::UInt16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plInt32")
  {
    TestIntegerVariant<plInt32>(plVariant::Type::Int32);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plUInt32")
  {
    TestIntegerVariant<plUInt32>(plVariant::Type::UInt32);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plInt64")
  {
    TestIntegerVariant<plInt64>(plVariant::Type::Int64);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plUInt64")
  {
    TestIntegerVariant<plUInt64>(plVariant::Type::UInt64);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "float")
  {
    plVariant b(42.0f);
    TestVariant<float>(b, plVariantType::Float);

    PLASMA_TEST_BOOL(b.Get<float>() == 42.0f);

    PLASMA_TEST_BOOL(b == plVariant(42));
    PLASMA_TEST_BOOL(b != plVariant(11));
    PLASMA_TEST_BOOL(b == plVariant(42.0));
    PLASMA_TEST_BOOL(b != plVariant(11.0));
    PLASMA_TEST_BOOL(b == plVariant(42.0f));
    PLASMA_TEST_BOOL(b != plVariant(11.0f));

    PLASMA_TEST_BOOL(b == 42);
    PLASMA_TEST_BOOL(b != 41);
    PLASMA_TEST_BOOL(b == 42.0);
    PLASMA_TEST_BOOL(b != 41.0);
    PLASMA_TEST_BOOL(b == 42.0f);
    PLASMA_TEST_BOOL(b != 41.0f);

    b = 17.0f;
    PLASMA_TEST_BOOL(b == 17.0f);

    b = plVariant(19.0f);
    PLASMA_TEST_BOOL(b == 19.0f);

    PLASMA_TEST_BOOL(b.IsNumber());
    PLASMA_TEST_BOOL(!b.IsString());
    PLASMA_TEST_BOOL(b.IsFloatingPoint());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "double")
  {
    plVariant b(42.0);
    TestVariant<double>(b, plVariantType::Double);
    PLASMA_TEST_BOOL(b.Get<double>() == 42.0);

    PLASMA_TEST_BOOL(b == plVariant(42));
    PLASMA_TEST_BOOL(b != plVariant(11));
    PLASMA_TEST_BOOL(b == plVariant(42.0));
    PLASMA_TEST_BOOL(b != plVariant(11.0));
    PLASMA_TEST_BOOL(b == plVariant(42.0f));
    PLASMA_TEST_BOOL(b != plVariant(11.0f));

    PLASMA_TEST_BOOL(b == 42);
    PLASMA_TEST_BOOL(b != 41);
    PLASMA_TEST_BOOL(b == 42.0);
    PLASMA_TEST_BOOL(b != 41.0);
    PLASMA_TEST_BOOL(b == 42.0f);
    PLASMA_TEST_BOOL(b != 41.0f);

    b = 17.0;
    PLASMA_TEST_BOOL(b == 17.0);

    b = plVariant(19.0);
    PLASMA_TEST_BOOL(b == 19.0);

    PLASMA_TEST_BOOL(b.IsNumber());
    PLASMA_TEST_BOOL(!b.IsString());
    PLASMA_TEST_BOOL(b.IsFloatingPoint());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plColor")
  {
    plVariant v(plColor(1, 2, 3, 1));
    TestVariant<plColor>(v, plVariantType::Color);

    PLASMA_TEST_BOOL(v.CanConvertTo<plColorGammaUB>());
    PLASMA_TEST_BOOL(v.ConvertTo<plColorGammaUB>() == static_cast<plColorGammaUB>(plColor(1, 2, 3, 1)));
    PLASMA_TEST_BOOL(v.Get<plColor>() == plColor(1, 2, 3, 1));

    PLASMA_TEST_BOOL(v == plVariant(plColor(1, 2, 3)));
    PLASMA_TEST_BOOL(v != plVariant(plColor(1, 1, 1)));

    PLASMA_TEST_BOOL(v == plColor(1, 2, 3));
    PLASMA_TEST_BOOL(v != plColor(1, 4, 3));

    v = plColor(5, 8, 9);
    PLASMA_TEST_BOOL(v == plColor(5, 8, 9));

    v = plVariant(plColor(7, 9, 4));
    PLASMA_TEST_BOOL(v == plColor(7, 9, 4));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v[2] == 4);
    PLASMA_TEST_BOOL(v[3] == 1);
    PLASMA_TEST_BOOL(v[4] == plVariant());
    PLASMA_TEST_BOOL(!v[4].IsValid());
    PLASMA_TEST_BOOL(v["r"] == 7);
    PLASMA_TEST_BOOL(v["g"] == 9);
    PLASMA_TEST_BOOL(v["b"] == 4);
    PLASMA_TEST_BOOL(v["a"] == 1);
    PLASMA_TEST_BOOL(v["x"] == plVariant());
    PLASMA_TEST_BOOL(!v["x"].IsValid());

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plColorGammaUB")
  {
    plVariant v(plColorGammaUB(64, 128, 255, 255));
    TestVariant<plColorGammaUB>(v, plVariantType::ColorGamma);

    PLASMA_TEST_BOOL(v.CanConvertTo<plColor>());
    PLASMA_TEST_BOOL(v.Get<plColorGammaUB>() == plColorGammaUB(64, 128, 255, 255));

    PLASMA_TEST_BOOL(v == plVariant(plColorGammaUB(64, 128, 255, 255)));
    PLASMA_TEST_BOOL(v != plVariant(plColorGammaUB(255, 128, 255, 255)));

    PLASMA_TEST_BOOL(v == plColorGammaUB(64, 128, 255, 255));
    PLASMA_TEST_BOOL(v != plColorGammaUB(64, 42, 255, 255));

    v = plColorGammaUB(10, 50, 200);
    PLASMA_TEST_BOOL(v == plColorGammaUB(10, 50, 200));

    v = plVariant(plColorGammaUB(17, 120, 200));
    PLASMA_TEST_BOOL(v == plColorGammaUB(17, 120, 200));
    PLASMA_TEST_BOOL(v[0] == 17);
    PLASMA_TEST_BOOL(v[1] == 120);
    PLASMA_TEST_BOOL(v[2] == 200);
    PLASMA_TEST_BOOL(v[3] == 255);
    PLASMA_TEST_BOOL(v[4] == plVariant());
    PLASMA_TEST_BOOL(!v[4].IsValid());
    PLASMA_TEST_BOOL(v["r"] == 17);
    PLASMA_TEST_BOOL(v["g"] == 120);
    PLASMA_TEST_BOOL(v["b"] == 200);
    PLASMA_TEST_BOOL(v["a"] == 255);
    PLASMA_TEST_BOOL(v["x"] == plVariant());
    PLASMA_TEST_BOOL(!v["x"].IsValid());

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec2")
  {
    plVariant v(plVec2(1, 2));
    TestVariant<plVec2>(v, plVariantType::Vector2);

    PLASMA_TEST_BOOL(v.Get<plVec2>() == plVec2(1, 2));

    PLASMA_TEST_BOOL(v == plVariant(plVec2(1, 2)));
    PLASMA_TEST_BOOL(v != plVariant(plVec2(1, 1)));

    PLASMA_TEST_BOOL(v == plVec2(1, 2));
    PLASMA_TEST_BOOL(v != plVec2(1, 4));

    v = plVec2(5, 8);
    PLASMA_TEST_BOOL(v == plVec2(5, 8));

    v = plVariant(plVec2(7, 9));
    PLASMA_TEST_BOOL(v == plVec2(7, 9));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec3")
  {
    plVariant v(plVec3(1, 2, 3));
    TestVariant<plVec3>(v, plVariantType::Vector3);

    PLASMA_TEST_BOOL(v.Get<plVec3>() == plVec3(1, 2, 3));

    PLASMA_TEST_BOOL(v == plVariant(plVec3(1, 2, 3)));
    PLASMA_TEST_BOOL(v != plVariant(plVec3(1, 1, 3)));

    PLASMA_TEST_BOOL(v == plVec3(1, 2, 3));
    PLASMA_TEST_BOOL(v != plVec3(1, 4, 3));

    v = plVec3(5, 8, 9);
    PLASMA_TEST_BOOL(v == plVec3(5, 8, 9));

    v = plVariant(plVec3(7, 9, 8));
    PLASMA_TEST_BOOL(v == plVec3(7, 9, 8));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v[2] == 8);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);
    PLASMA_TEST_BOOL(v["z"] == 8);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec4")
  {
    plVariant v(plVec4(1, 2, 3, 4));
    TestVariant<plVec4>(v, plVariantType::Vector4);

    PLASMA_TEST_BOOL(v.Get<plVec4>() == plVec4(1, 2, 3, 4));

    PLASMA_TEST_BOOL(v == plVariant(plVec4(1, 2, 3, 4)));
    PLASMA_TEST_BOOL(v != plVariant(plVec4(1, 1, 3, 4)));

    PLASMA_TEST_BOOL(v == plVec4(1, 2, 3, 4));
    PLASMA_TEST_BOOL(v != plVec4(1, 4, 3, 4));

    v = plVec4(5, 8, 9, 3);
    PLASMA_TEST_BOOL(v == plVec4(5, 8, 9, 3));

    v = plVariant(plVec4(7, 9, 8, 4));
    PLASMA_TEST_BOOL(v == plVec4(7, 9, 8, 4));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v[2] == 8);
    PLASMA_TEST_BOOL(v[3] == 4);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);
    PLASMA_TEST_BOOL(v["z"] == 8);
    PLASMA_TEST_BOOL(v["w"] == 4);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec2I32")
  {
    plVariant v(plVec2I32(1, 2));
    TestVariant<plVec2I32>(v, plVariantType::Vector2I);

    PLASMA_TEST_BOOL(v.Get<plVec2I32>() == plVec2I32(1, 2));

    PLASMA_TEST_BOOL(v == plVariant(plVec2I32(1, 2)));
    PLASMA_TEST_BOOL(v != plVariant(plVec2I32(1, 1)));

    PLASMA_TEST_BOOL(v == plVec2I32(1, 2));
    PLASMA_TEST_BOOL(v != plVec2I32(1, 4));

    v = plVec2I32(5, 8);
    PLASMA_TEST_BOOL(v == plVec2I32(5, 8));

    v = plVariant(plVec2I32(7, 9));
    PLASMA_TEST_BOOL(v == plVec2I32(7, 9));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec3I32")
  {
    plVariant v(plVec3I32(1, 2, 3));
    TestVariant<plVec3I32>(v, plVariantType::Vector3I);

    PLASMA_TEST_BOOL(v.Get<plVec3I32>() == plVec3I32(1, 2, 3));

    PLASMA_TEST_BOOL(v == plVariant(plVec3I32(1, 2, 3)));
    PLASMA_TEST_BOOL(v != plVariant(plVec3I32(1, 1, 3)));

    PLASMA_TEST_BOOL(v == plVec3I32(1, 2, 3));
    PLASMA_TEST_BOOL(v != plVec3I32(1, 4, 3));

    v = plVec3I32(5, 8, 9);
    PLASMA_TEST_BOOL(v == plVec3I32(5, 8, 9));

    v = plVariant(plVec3I32(7, 9, 8));
    PLASMA_TEST_BOOL(v == plVec3I32(7, 9, 8));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v[2] == 8);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);
    PLASMA_TEST_BOOL(v["z"] == 8);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVec4I32")
  {
    plVariant v(plVec4I32(1, 2, 3, 4));
    TestVariant<plVec4I32>(v, plVariantType::Vector4I);

    PLASMA_TEST_BOOL(v.Get<plVec4I32>() == plVec4I32(1, 2, 3, 4));

    PLASMA_TEST_BOOL(v == plVariant(plVec4I32(1, 2, 3, 4)));
    PLASMA_TEST_BOOL(v != plVariant(plVec4I32(1, 1, 3, 4)));

    PLASMA_TEST_BOOL(v == plVec4I32(1, 2, 3, 4));
    PLASMA_TEST_BOOL(v != plVec4I32(1, 4, 3, 4));

    v = plVec4I32(5, 8, 9, 3);
    PLASMA_TEST_BOOL(v == plVec4I32(5, 8, 9, 3));

    v = plVariant(plVec4I32(7, 9, 8, 4));
    PLASMA_TEST_BOOL(v == plVec4I32(7, 9, 8, 4));
    PLASMA_TEST_BOOL(v[0] == 7);
    PLASMA_TEST_BOOL(v[1] == 9);
    PLASMA_TEST_BOOL(v[2] == 8);
    PLASMA_TEST_BOOL(v[3] == 4);
    PLASMA_TEST_BOOL(v["x"] == 7);
    PLASMA_TEST_BOOL(v["y"] == 9);
    PLASMA_TEST_BOOL(v["z"] == 8);
    PLASMA_TEST_BOOL(v["w"] == 4);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plQuat")
  {
    plVariant v(plQuat(1, 2, 3, 4));
    TestVariant<plQuat>(v, plVariantType::Quaternion);

    PLASMA_TEST_BOOL(v.Get<plQuat>() == plQuat(1, 2, 3, 4));

    PLASMA_TEST_BOOL(v == plQuat(1, 2, 3, 4));
    PLASMA_TEST_BOOL(v != plQuat(1, 2, 3, 5));

    PLASMA_TEST_BOOL(v == plQuat(1, 2, 3, 4));
    PLASMA_TEST_BOOL(v != plQuat(1, 4, 3, 4));

    v = plQuat(5, 8, 9, 3);
    PLASMA_TEST_BOOL(v == plQuat(5, 8, 9, 3));

    v = plVariant(plQuat(7, 9, 8, 4));
    PLASMA_TEST_BOOL(v == plQuat(7, 9, 8, 4));
    
    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);

    plTypedPointer ptr = v.GetWriteAccess();
    PLASMA_TEST_BOOL(ptr.m_pObject == &v.Get<plQuat>());
    PLASMA_TEST_BOOL(ptr.m_pObject == &v.GetWritable<plQuat>());
    PLASMA_TEST_BOOL(ptr.m_pType == plGetStaticRTTI<plQuat>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMat3")
  {
    plVariant v(plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    TestVariant<plMat3>(v, plVariantType::Matrix3);

    PLASMA_TEST_BOOL(v.Get<plMat3>() == plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));

    PLASMA_TEST_BOOL(v == plVariant(plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    PLASMA_TEST_BOOL(v != plVariant(plMat3(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    PLASMA_TEST_BOOL(v == plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    PLASMA_TEST_BOOL(v != plMat3(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = plMat3(5, 8, 9, 3, 1, 2, 3, 4, 5);
    PLASMA_TEST_BOOL(v == plMat3(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = plVariant(plMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));
    PLASMA_TEST_BOOL(v == plMat3(5, 8, 9, 3, 1, 2, 3, 4, 4));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMat4")
  {
    plVariant v(plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    TestVariant<plMat4>(v, plVariantType::Matrix4);

    PLASMA_TEST_BOOL(v.Get<plMat4>() == plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    PLASMA_TEST_BOOL(v == plVariant(plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    PLASMA_TEST_BOOL(v != plVariant(plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    PLASMA_TEST_BOOL(v == plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    PLASMA_TEST_BOOL(v != plMat4(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = plMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    PLASMA_TEST_BOOL(v == plMat4(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = plVariant(plMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    PLASMA_TEST_BOOL(v == plMat4(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTransform")
  {
    plVariant v(plTransform(plVec3(1, 2, 3), plQuat(4, 5, 6, 7), plVec3(8, 9, 10)));
    TestVariant<plTransform>(v, plVariantType::Transform);

    PLASMA_TEST_BOOL(v.Get<plTransform>() == plTransform(plVec3(1, 2, 3), plQuat(4, 5, 6, 7), plVec3(8, 9, 10)));

    PLASMA_TEST_BOOL(v == plTransform(plVec3(1, 2, 3), plQuat(4, 5, 6, 7), plVec3(8, 9, 10)));
    PLASMA_TEST_BOOL(v != plTransform(plVec3(1, 2, 3), plQuat(4, 5, 6, 7), plVec3(8, 9, 11)));

    v = plTransform(plVec3(5, 8, 9), plQuat(3, 1, 2, 3), plVec3(4, 5, 3));
    PLASMA_TEST_BOOL(v == plTransform(plVec3(5, 8, 9), plQuat(3, 1, 2, 3), plVec3(4, 5, 3)));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "const char*")
  {
    plVariant v("This is a const char array");
    TestVariant<plString>(v, plVariantType::String);

    PLASMA_TEST_BOOL(v.IsA<const char*>());
    PLASMA_TEST_BOOL(v.IsA<char*>());
    PLASMA_TEST_BOOL(v.Get<plString>() == plString("This is a const char array"));

    PLASMA_TEST_BOOL(v == plVariant("This is a const char array"));
    PLASMA_TEST_BOOL(v != plVariant("This is something else"));

    PLASMA_TEST_BOOL(v == plString("This is a const char array"));
    PLASMA_TEST_BOOL(v != plString("This is another string"));

    PLASMA_TEST_BOOL(v == "This is a const char array");
    PLASMA_TEST_BOOL(v != "This is another string");

    PLASMA_TEST_BOOL(v == (const char*)"This is a const char array");
    PLASMA_TEST_BOOL(v != (const char*)"This is another string");

    v = "blurg!";
    PLASMA_TEST_BOOL(v == plString("blurg!"));

    v = plVariant("blärg!");
    PLASMA_TEST_BOOL(v == plString("blärg!"));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plString")
  {
    plVariant v(plString("This is an plString"));
    TestVariant<plString>(v, plVariantType::String);

    PLASMA_TEST_BOOL(v.Get<plString>() == plString("This is an plString"));

    PLASMA_TEST_BOOL(v == plVariant(plString("This is an plString")));
    PLASMA_TEST_BOOL(v != plVariant(plString("This is something else")));

    PLASMA_TEST_BOOL(v == plString("This is an plString"));
    PLASMA_TEST_BOOL(v != plString("This is another plString"));

    v = plString("blurg!");
    PLASMA_TEST_BOOL(v == plString("blurg!"));

    v = plVariant(plString("blärg!"));
    PLASMA_TEST_BOOL(v == plString("blärg!"));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plStringView")
  {
    const char* szTemp = "This is an plStringView";
    plStringView bla(szTemp);
    plVariant v(bla, false);
    TestVariant<plStringView>(v, plVariantType::StringView);

    const plString sCopy = szTemp;
    PLASMA_TEST_BOOL(v.Get<plStringView>() == sCopy);

    PLASMA_TEST_BOOL(v == plVariant(plStringView(sCopy.GetData()), false));
    PLASMA_TEST_BOOL(v != plVariant(plStringView("This is something else"), false));

    PLASMA_TEST_BOOL(v == plStringView(sCopy.GetData()));
    PLASMA_TEST_BOOL(v != plStringView("This is something else"));

    v = plVariant(plStringView("blurg!"), false);
    PLASMA_TEST_BOOL(v == plStringView("blurg!"));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plDataBuffer")
  {
    plDataBuffer a, a2;
    a.PushBack(plUInt8(1));
    a.PushBack(plUInt8(2));
    a.PushBack(plUInt8(255));

    plVariant va(a);
    TestVariant<plDataBuffer>(va, plVariantType::DataBuffer);

    const plDataBuffer& b = va.Get<plDataBuffer>();
    plArrayPtr<const plUInt8> b2 = va.Get<plDataBuffer>();

    PLASMA_TEST_BOOL(a == b);
    PLASMA_TEST_BOOL(a == b2);

    PLASMA_TEST_BOOL(a != a2);

    PLASMA_TEST_BOOL(va == a);
    PLASMA_TEST_BOOL(va != a2);

    PLASMA_TEST_BOOL(va.IsNumber() == false);
    PLASMA_TEST_BOOL(!va.IsString());
    PLASMA_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTime")
  {
    plVariant v(plTime::Seconds(1337));
    TestVariant<plTime>(v, plVariantType::Time);

    PLASMA_TEST_BOOL(v.Get<plTime>() == plTime::Seconds(1337));

    PLASMA_TEST_BOOL(v == plVariant(plTime::Seconds(1337)));
    PLASMA_TEST_BOOL(v != plVariant(plTime::Seconds(1336)));

    PLASMA_TEST_BOOL(v == plTime::Seconds(1337));
    PLASMA_TEST_BOOL(v != plTime::Seconds(1338));

    v = plTime::Seconds(8472);
    PLASMA_TEST_BOOL(v == plTime::Seconds(8472));

    v = plVariant(plTime::Seconds(13));
    PLASMA_TEST_BOOL(v == plTime::Seconds(13));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plUuid")
  {
    plUuid id;
    plVariant v(id);
    TestVariant<plUuid>(v, plVariantType::Uuid);

    PLASMA_TEST_BOOL(v.Get<plUuid>() == plUuid());

    plUuid uuid;
    uuid.CreateNewUuid();
    PLASMA_TEST_BOOL(v != plVariant(uuid));
    PLASMA_TEST_BOOL(plVariant(uuid).Get<plUuid>() == uuid);

    plUuid uuid2;
    uuid2.CreateNewUuid();
    PLASMA_TEST_BOOL(plVariant(uuid) != plVariant(uuid2));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plAngle")
  {
    plVariant v(plAngle::Degree(1337));
    TestVariant<plAngle>(v, plVariantType::Angle);

    PLASMA_TEST_BOOL(v.Get<plAngle>() == plAngle::Degree(1337));

    PLASMA_TEST_BOOL(v == plVariant(plAngle::Degree(1337)));
    PLASMA_TEST_BOOL(v != plVariant(plAngle::Degree(1336)));

    PLASMA_TEST_BOOL(v == plAngle::Degree(1337));
    PLASMA_TEST_BOOL(v != plAngle::Degree(1338));

    v = plAngle::Degree(8472);
    PLASMA_TEST_BOOL(v == plAngle::Degree(8472));

    v = plVariant(plAngle::Degree(13));
    PLASMA_TEST_BOOL(v == plAngle::Degree(13));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plHashedString")
  {
    plVariant v(plMakeHashedString("ABCDE"));
    TestVariant<plHashedString>(v, plVariantType::HashedString);

    PLASMA_TEST_BOOL(v.Get<plHashedString>() == plMakeHashedString("ABCDE"));

    PLASMA_TEST_BOOL(v == plVariant(plMakeHashedString("ABCDE")));
    PLASMA_TEST_BOOL(v != plVariant(plMakeHashedString("ABCDK")));
    PLASMA_TEST_BOOL(v == plVariant(plTempHashedString("ABCDE")));
    PLASMA_TEST_BOOL(v != plVariant(plTempHashedString("ABCDK")));

    PLASMA_TEST_BOOL(v == plMakeHashedString("ABCDE"));
    PLASMA_TEST_BOOL(v != plMakeHashedString("ABCDK"));
    PLASMA_TEST_BOOL(v == plTempHashedString("ABCDE"));
    PLASMA_TEST_BOOL(v != plTempHashedString("ABCDK"));

    v = plMakeHashedString("HHH");
    PLASMA_TEST_BOOL(v == plMakeHashedString("HHH"));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(v.IsString() == false);
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTempHashedString")
  {
    plVariant v(plTempHashedString("ABCDE"));
    TestVariant<plTempHashedString>(v, plVariantType::TempHashedString);

    PLASMA_TEST_BOOL(v.Get<plTempHashedString>() == plTempHashedString("ABCDE"));

    PLASMA_TEST_BOOL(v == plVariant(plTempHashedString("ABCDE")));
    PLASMA_TEST_BOOL(v != plVariant(plTempHashedString("ABCDK")));
    PLASMA_TEST_BOOL(v == plVariant(plMakeHashedString("ABCDE")));
    PLASMA_TEST_BOOL(v != plVariant(plMakeHashedString("ABCDK")));

    PLASMA_TEST_BOOL(v == plTempHashedString("ABCDE"));
    PLASMA_TEST_BOOL(v != plTempHashedString("ABCDK"));
    PLASMA_TEST_BOOL(v == plMakeHashedString("ABCDE"));
    PLASMA_TEST_BOOL(v != plMakeHashedString("ABCDK"));

    v = plTempHashedString("HHH");
    PLASMA_TEST_BOOL(v == plTempHashedString("HHH"));

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(v.IsString() == false);
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVariantArray")
  {
    plVariantArray a, a2;
    a.PushBack("This");
    a.PushBack("is a");
    a.PushBack("test");

    plVariant va(a);
    PLASMA_TEST_BOOL(va.IsValid());
    PLASMA_TEST_BOOL(va.GetType() == plVariant::Type::VariantArray);
    PLASMA_TEST_BOOL(va.IsA<plVariantArray>());
    PLASMA_TEST_BOOL(va.GetReflectedType() == nullptr);

    const plArrayPtr<const plVariant>& b = va.Get<plVariantArray>();
    plArrayPtr<const plVariant> b2 = va.Get<plVariantArray>();

    PLASMA_TEST_BOOL(a == b);
    PLASMA_TEST_BOOL(a == b2);

    PLASMA_TEST_BOOL(a != a2);

    PLASMA_TEST_BOOL(va == a);
    PLASMA_TEST_BOOL(va != a2);

    PLASMA_TEST_BOOL(va[0] == plString("This"));
    PLASMA_TEST_BOOL(va[1] == plString("is a"));
    PLASMA_TEST_BOOL(va[2] == plString("test"));
    PLASMA_TEST_BOOL(va[4] == plVariant());
    PLASMA_TEST_BOOL(!va[4].IsValid());

    PLASMA_TEST_BOOL(va.IsNumber() == false);
    PLASMA_TEST_BOOL(!va.IsString());
    PLASMA_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plVariantDictionary")
  {
    plVariantDictionary a, a2;
    a["my"] = true;
    a["luv"] = 4;
    a["pon"] = "ies";

    plVariant va(a);
    PLASMA_TEST_BOOL(va.IsValid());
    PLASMA_TEST_BOOL(va.GetType() == plVariant::Type::VariantDictionary);
    PLASMA_TEST_BOOL(va.IsA<plVariantDictionary>());
    PLASMA_TEST_BOOL(va.GetReflectedType() == nullptr);

    const plVariantDictionary& d1 = va.Get<plVariantDictionary>();
    plVariantDictionary d2 = va.Get<plVariantDictionary>();

    PLASMA_TEST_BOOL(a == d1);
    PLASMA_TEST_BOOL(a == d2);
    PLASMA_TEST_BOOL(d1 == d2);

    PLASMA_TEST_BOOL(va == a);
    PLASMA_TEST_BOOL(va != a2);

    PLASMA_TEST_BOOL(va["my"] == true);
    PLASMA_TEST_BOOL(va["luv"] == 4);
    PLASMA_TEST_BOOL(va["pon"] == plString("ies"));
    PLASMA_TEST_BOOL(va["x"] == plVariant());
    PLASMA_TEST_BOOL(!va["x"].IsValid());

    PLASMA_TEST_BOOL(va.IsNumber() == false);
    PLASMA_TEST_BOOL(!va.IsString());
    PLASMA_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTypedPointer")
  {
    Blubb blubb;
    blubb.u = 1.0f;
    blubb.v = 2.0f;

    Blubb blubb2;

    plVariant v(&blubb);

    PLASMA_TEST_BOOL(v.IsValid());
    PLASMA_TEST_BOOL(v.GetType() == plVariant::Type::TypedPointer);
    PLASMA_TEST_BOOL(v.IsA<Blubb*>());
    PLASMA_TEST_BOOL(v.Get<Blubb*>() == &blubb);
    PLASMA_TEST_BOOL(v.IsA<plReflectedClass*>());
    PLASMA_TEST_BOOL(v.Get<plReflectedClass*>() == &blubb);
    PLASMA_TEST_BOOL(v.Get<plReflectedClass*>() != &blubb2);
    PLASMA_TEST_BOOL(plDynamicCast<Blubb*>(v) == &blubb);
    PLASMA_TEST_BOOL(plDynamicCast<plVec3*>(v) == nullptr);
    PLASMA_TEST_BOOL(v.IsA<void*>());
    PLASMA_TEST_BOOL(v.Get<void*>() == &blubb);
    PLASMA_TEST_BOOL(v.IsA<const void*>());
    PLASMA_TEST_BOOL(v.Get<const void*>() == &blubb);
    PLASMA_TEST_BOOL(v.GetData() == &blubb);
    PLASMA_TEST_BOOL(v.IsA<plTypedPointer>());
    PLASMA_TEST_BOOL(v.GetReflectedType() == plGetStaticRTTI<Blubb>());
    PLASMA_TEST_BOOL(!v.IsA<plVec3*>());

    plTypedPointer ptr = v.Get<plTypedPointer>();
    PLASMA_TEST_BOOL(ptr.m_pObject == &blubb);
    PLASMA_TEST_BOOL(ptr.m_pType == plGetStaticRTTI<Blubb>());

    plTypedPointer ptr2 = v.GetWriteAccess();
    PLASMA_TEST_BOOL(ptr2.m_pObject == &blubb);
    PLASMA_TEST_BOOL(ptr2.m_pType == plGetStaticRTTI<Blubb>());

    PLASMA_TEST_BOOL(v[0] == 1.0f);
    PLASMA_TEST_BOOL(v[1] == 2.0f);
    PLASMA_TEST_BOOL(v["u"] == 1.0f);
    PLASMA_TEST_BOOL(v["v"] == 2.0f);
    plVariant v2 = &blubb;
    PLASMA_TEST_BOOL(v == v2);
    plVariant v3 = ptr;
    PLASMA_TEST_BOOL(v == v3);

    PLASMA_TEST_BOOL(v.IsNumber() == false);
    PLASMA_TEST_BOOL(!v.IsString());
    PLASMA_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTypedPointer nullptr")
  {
    plTypedPointer ptr = {nullptr, plGetStaticRTTI<Blubb>()};
    plVariant v = ptr;
    PLASMA_TEST_BOOL(v.IsValid());
    PLASMA_TEST_BOOL(v.GetType() == plVariant::Type::TypedPointer);
    PLASMA_TEST_BOOL(v.IsA<Blubb*>());
    PLASMA_TEST_BOOL(v.Get<Blubb*>() == nullptr);
    PLASMA_TEST_BOOL(v.IsA<plReflectedClass*>());
    PLASMA_TEST_BOOL(v.Get<plReflectedClass*>() == nullptr);
    PLASMA_TEST_BOOL(plDynamicCast<Blubb*>(v) == nullptr);
    PLASMA_TEST_BOOL(plDynamicCast<plVec3*>(v) == nullptr);
    PLASMA_TEST_BOOL(v.IsA<void*>());
    PLASMA_TEST_BOOL(v.Get<void*>() == nullptr);
    PLASMA_TEST_BOOL(v.IsA<const void*>());
    PLASMA_TEST_BOOL(v.Get<const void*>() == nullptr);
    PLASMA_TEST_BOOL(v.IsA<plTypedPointer>());
    PLASMA_TEST_BOOL(v.GetReflectedType() == plGetStaticRTTI<Blubb>());
    PLASMA_TEST_BOOL(!v.IsA<plVec3*>());

    plTypedPointer ptr2 = v.Get<plTypedPointer>();
    PLASMA_TEST_BOOL(ptr2.m_pObject == nullptr);
    PLASMA_TEST_BOOL(ptr2.m_pType == plGetStaticRTTI<Blubb>());

    PLASMA_TEST_BOOL(!v[0].IsValid());
    PLASMA_TEST_BOOL(!v["u"].IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTypedObject inline")
  {
    // plAngle::MakeFromDegree(90.0f) was replaced with radian as release builds generate a different float then debug.
    plVarianceTypeAngle value = {0.1f, plAngle::Radian(1.57079637f)};
    plVarianceTypeAngle value2 = {0.2f, plAngle::Radian(1.57079637f)};

    plVariant v(value);
    TestVariant<plVarianceTypeAngle>(v, plVariantType::TypedObject);

    PLASMA_TEST_BOOL(v.IsA<plTypedObject>());
    PLASMA_TEST_BOOL(!v.IsA<void*>());
    PLASMA_TEST_BOOL(!v.IsA<const void*>());
    PLASMA_TEST_BOOL(!v.IsA<plVec3*>());
    PLASMA_TEST_BOOL(plDynamicCast<plVec3*>(v) == nullptr);

    const plVarianceTypeAngle& valueGet = v.Get<plVarianceTypeAngle>();
    PLASMA_TEST_BOOL(value == valueGet);

    plVariant va = value;
    PLASMA_TEST_BOOL(v == va);

    plVariant v2 = value2;
    PLASMA_TEST_BOOL(v != v2);

    plUInt64 uiHash = v.ComputeHash(0);
    PLASMA_TEST_INT(uiHash, 13667342936068485827ul);

    plVarianceTypeAngle* pTypedAngle = PLASMA_DEFAULT_NEW(plVarianceTypeAngle, {0.1f, plAngle::Radian(1.57079637f)});
    plVariant copy;
    copy.CopyTypedObject(pTypedAngle, plGetStaticRTTI<plVarianceTypeAngle>());
    plVariant move;
    move.MoveTypedObject(pTypedAngle, plGetStaticRTTI<plVarianceTypeAngle>());
    PLASMA_TEST_BOOL(v == copy);
    PLASMA_TEST_BOOL(v == move);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTypedObject shared")
  {
    plTypedObjectStruct data;
    plVariant v = data;
    PLASMA_TEST_BOOL(v.IsValid());
    PLASMA_TEST_BOOL(v.GetType() == plVariant::Type::TypedObject);
    PLASMA_TEST_BOOL(v.IsA<plTypedObject>());
    PLASMA_TEST_BOOL(v.IsA<plTypedObjectStruct>());
    PLASMA_TEST_BOOL(!v.IsA<void*>());
    PLASMA_TEST_BOOL(!v.IsA<const void*>());
    PLASMA_TEST_BOOL(!v.IsA<plVec3*>());
    PLASMA_TEST_BOOL(plDynamicCast<plVec3*>(v) == nullptr);
    PLASMA_TEST_BOOL(v.GetReflectedType() == plGetStaticRTTI<plTypedObjectStruct>());

    plVariant v2 = v;

    plTypedPointer ptr = v.GetWriteAccess();
    PLASMA_TEST_BOOL(ptr.m_pObject == &v.Get<plTypedObjectStruct>());
    PLASMA_TEST_BOOL(ptr.m_pObject == &v.GetWritable<plTypedObjectStruct>());
    PLASMA_TEST_BOOL(ptr.m_pObject != &v2.Get<plTypedObjectStruct>());
    PLASMA_TEST_BOOL(ptr.m_pType == plGetStaticRTTI<plTypedObjectStruct>());

    PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(ptr.m_pObject, &v2.Get<plTypedObjectStruct>(), plGetStaticRTTI<plTypedObjectStruct>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (bool)")
  {
    plVariant v(true);

    PLASMA_TEST_BOOL(v.CanConvertTo<bool>());
    PLASMA_TEST_BOOL(v.CanConvertTo<plInt32>());

    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Invalid) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Bool));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Float));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Double));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Color) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Quaternion) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::String));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::StringView) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::DataBuffer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Time) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Angle) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantArray) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantDictionary) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedPointer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedObject) == false);

    PLASMA_TEST_BOOL(v.ConvertTo<bool>() == true);
    PLASMA_TEST_BOOL(v.ConvertTo<plInt8>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plUInt8>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plInt16>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plUInt16>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plInt32>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plUInt32>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plInt64>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<plUInt64>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo<float>() == 1.0f);
    PLASMA_TEST_BOOL(v.ConvertTo<double>() == 1.0);
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "true");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("true"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("true"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Bool).Get<bool>() == true);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int8).Get<plInt8>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt8).Get<plUInt8>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int16).Get<plInt16>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt16).Get<plUInt16>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int32).Get<plInt32>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt32).Get<plUInt32>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int64).Get<plInt64>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt64).Get<plUInt64>() == 1);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Float).Get<float>() == 1.0f);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Double).Get<double>() == 1.0);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "true");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("true"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("true"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plInt8)")
  {
    plVariant v((plInt8)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plUInt8)")
  {
    plVariant v((plUInt8)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plInt16)")
  {
    plVariant v((plInt16)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plUInt16)")
  {
    plVariant v((plUInt16)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plInt32)")
  {
    plVariant v((plInt32)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plUInt32)")
  {
    plVariant v((plUInt32)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plInt64)")
  {
    plVariant v((plInt64)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plUInt64)")
  {
    plVariant v((plUInt64)3);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (float)")
  {
    plVariant v((float)3.0f);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (double)")
  {
    plVariant v((double)3.0f);
    TestNumberCanConvertTo(v);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (Color)")
  {
    plColor c(3, 3, 4, 0);
    plVariant v(c);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Color, plVariant::Type::ColorGamma);

    plResult conversionResult = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<plColor>(&conversionResult) == c);
    PLASMA_TEST_BOOL(conversionResult.Succeeded());

    PLASMA_TEST_BOOL(v.ConvertTo<plString>(&conversionResult) == "{ r=3, g=3, b=4, a=0 }");
    PLASMA_TEST_BOOL(conversionResult.Succeeded());

    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ r=3, g=3, b=4, a=0 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Color).Get<plColor>() == c);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ r=3, g=3, b=4, a=0 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ r=3, g=3, b=4, a=0 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (ColorGamma)")
  {
    plColorGammaUB c(0, 128, 64, 255);
    plVariant v(c);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::ColorGamma, plVariant::Type::Color);

    plResult conversionResult = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<plColorGammaUB>(&conversionResult) == c);
    PLASMA_TEST_BOOL(conversionResult.Succeeded());

    plString val = v.ConvertTo<plString>(&conversionResult);
    PLASMA_TEST_BOOL(val == "{ r=0, g=128, b=64, a=255 }");
    PLASMA_TEST_BOOL(conversionResult.Succeeded());

    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ r=0, g=128, b=64, a=255 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::ColorGamma).Get<plColorGammaUB>() == c);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ r=0, g=128, b=64, a=255 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ r=0, g=128, b=64, a=255 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec2)")
  {
    plVec2 vec(3.0f, 4.0f);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector2, plVariant::Type::Vector2I, plVariant::Type::Vector2U);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec2>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec2I32>() == plVec2I32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec2U32>() == plVec2U32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2).Get<plVec2>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2I).Get<plVec2I32>() == plVec2I32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2U).Get<plVec2U32>() == plVec2U32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec3)")
  {
    plVec3 vec(3.0f, 4.0f, 6.0f);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector3, plVariant::Type::Vector3I, plVariant::Type::Vector3U);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec3>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec3I32>() == plVec3I32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec3U32>() == plVec3U32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4, z=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=6 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3).Get<plVec3>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3I).Get<plVec3I32>() == plVec3I32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3U).Get<plVec3U32>() == plVec3U32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4, z=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=6 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec4)")
  {
    plVec4 vec(3.0f, 4.0f, 3, 56);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector4, plVariant::Type::Vector4I, plVariant::Type::Vector4U);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec4>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec4I32>() == plVec4I32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec4U32>() == plVec4U32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4).Get<plVec4>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4I).Get<plVec4I32>() == plVec4I32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4U).Get<plVec4U32>() == plVec4U32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec2I32)")
  {
    plVec2I32 vec(3, 4);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector2I, plVariant::Type::Vector2U, plVariant::Type::Vector2);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec2I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec2>() == plVec2(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec2U32>() == plVec2U32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2I).Get<plVec2I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2).Get<plVec2>() == plVec2(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector2U).Get<plVec2U32>() == plVec2U32(3, 4));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec3I32)")
  {
    plVec3I32 vec(3, 4, 6);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector3I, plVariant::Type::Vector3U, plVariant::Type::Vector3);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec3I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec3>() == plVec3(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec3U32>() == plVec3U32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4, z=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=6 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3I).Get<plVec3I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3).Get<plVec3>() == plVec3(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector3U).Get<plVec3U32>() == plVec3U32(3, 4, 6));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4, z=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=6 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVec4I32)")
  {
    plVec4I32 vec(3, 4, 3, 56);
    plVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Vector4I, plVariant::Type::Vector4U, plVariant::Type::Vector4);

    PLASMA_TEST_BOOL(v.ConvertTo<plVec4I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo<plVec4>() == plVec4(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo<plVec4U32>() == plVec4U32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4I).Get<plVec4I32>() == vec);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4).Get<plVec4>() == plVec4(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Vector4U).Get<plVec4U32>() == plVec4U32(3, 4, 3, 56));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plQuat)")
  {
    plQuat q(3.0f, 4.0f, 3, 56);
    plVariant v(q);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Quaternion);

    PLASMA_TEST_BOOL(v.ConvertTo<plQuat>() == q);
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Quaternion).Get<plQuat>() == q);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ x=3, y=4, z=3, w=56 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plMat3)")
  {
    plMat3 m = plMat3(1, 2, 3, 4, 5, 6, 7, 8, 9);
    plVariant v(m);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Matrix3);

    PLASMA_TEST_BOOL(v.ConvertTo<plMat3>() == m);
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Matrix3).Get<plMat3>() == m);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plMat4)")
  {
    plMat4 m = plMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6);
    plVariant v(m);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Matrix4);

    PLASMA_TEST_BOOL(v.ConvertTo<plMat4>() == m);
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                            "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                            "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                            "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                     "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                     "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                     "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                         "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                         "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                         "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Matrix4).Get<plMat4>() == m);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                         "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                         "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                         "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                                                        "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                                                        "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                                                        "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                                                                "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                                                                "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                                                                "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plString)")
  {
    plVariant v("I don't feel like it anymore");

    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Invalid) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Bool));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Float));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Double));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Color) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Quaternion) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::String));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::StringView));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::DataBuffer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Time) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Angle) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::ColorGamma) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::HashedString));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TempHashedString));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantArray) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantDictionary) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedPointer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedObject) == false);

    {
      plResult ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt8>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt8>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt16>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt16>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt32>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt32>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt64>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt64>(&ConversionStatus) == 0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.0f);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.0);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>(&ConversionStatus) == plMakeHashedString("I don't feel like it anymore"));
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_SUCCESS;
      PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>(&ConversionStatus) == plTempHashedString("I don't feel like it anymore"));
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "true";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == true);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Bool, &ConversionStatus).Get<bool>() == true);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "-128";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt8>(&ConversionStatus) == -128);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int8, &ConversionStatus).Get<plInt8>() == -128);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "255";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt8>(&ConversionStatus) == 255);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt8, &ConversionStatus).Get<plUInt8>() == 255);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "-5643";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt16>(&ConversionStatus) == -5643);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int16, &ConversionStatus).Get<plInt16>() == -5643);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "9001";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt16>(&ConversionStatus) == 9001);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt16, &ConversionStatus).Get<plUInt16>() == 9001);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "46";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt32>(&ConversionStatus) == 46);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int32, &ConversionStatus).Get<plInt32>() == 46);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "356";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt32>(&ConversionStatus) == 356);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt32, &ConversionStatus).Get<plUInt32>() == 356);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "64";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plInt64>(&ConversionStatus) == 64);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Int64, &ConversionStatus).Get<plInt64>() == 64);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "6464";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<plUInt64>(&ConversionStatus) == 6464);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::UInt64, &ConversionStatus).Get<plUInt64>() == 6464);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "0.07564f";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.07564f);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Float, &ConversionStatus).Get<float>() == 0.07564f);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }

    {
      v = "0.4453";
      plResult ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.4453);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

      ConversionStatus = PLASMA_FAILURE;
      PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Double, &ConversionStatus).Get<double>() == 0.4453);
      PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plStringView)")
  {
    plStringView va0("Test String");
    plVariant v(va0, false);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::StringView);

    PLASMA_TEST_BOOL(v.ConvertTo<plStringView>() == va0);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::StringView).Get<plStringView>() == va0);

    {
      plVariant va, va2;

      va = "Bla";
      PLASMA_TEST_BOOL(va.IsA<plString>());
      PLASMA_TEST_BOOL(va.CanConvertTo<plString>());
      PLASMA_TEST_BOOL(va.CanConvertTo<plStringView>());

      va = plVariant("Bla"_plsv, false);
      PLASMA_TEST_BOOL(va.IsA<plStringView>());
      PLASMA_TEST_BOOL(va.CanConvertTo<plString>());
      PLASMA_TEST_BOOL(va.CanConvertTo<plStringView>());

      va2 = va;
      PLASMA_TEST_BOOL(va2.IsA<plStringView>());
      PLASMA_TEST_BOOL(va2.CanConvertTo<plString>());
      PLASMA_TEST_BOOL(va2.CanConvertTo<plStringView>());
      PLASMA_TEST_BOOL(va2.ConvertTo<plStringView>() == "Bla");
      PLASMA_TEST_BOOL(va2.ConvertTo<plString>() == "Bla");

      plVariant va3 = va2.ConvertTo(plVariantType::StringView);
      PLASMA_TEST_BOOL(va3.IsA<plStringView>());
      PLASMA_TEST_BOOL(va3.ConvertTo<plString>() == "Bla");

      va = "Blub";
      PLASMA_TEST_BOOL(va.IsA<plString>());

      plVariant va4 = va.ConvertTo(plVariantType::StringView);
      PLASMA_TEST_BOOL(va4.IsA<plStringView>());
      PLASMA_TEST_BOOL(va4.ConvertTo<plString>() == "Blub");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plDataBuffer)")
  {
    plDataBuffer va;
    va.PushBack(255);
    va.PushBack(4);
    plVariant v(va);

    TestCanOnlyConvertToID(v, plVariant::Type::DataBuffer);

    PLASMA_TEST_BOOL(v.ConvertTo<plDataBuffer>() == va);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::DataBuffer).Get<plDataBuffer>() == va);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plTime)")
  {
    plTime t = plTime::Seconds(123.0);
    plVariant v(t);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Time);

    PLASMA_TEST_BOOL(v.ConvertTo<plTime>() == t);
    // PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "");

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Time).Get<plTime>() == t);
    // PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plUuid)")
  {
    plUuid uuid;
    uuid.CreateNewUuid();
    plVariant v(uuid);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Uuid);

    PLASMA_TEST_BOOL(v.ConvertTo<plUuid>() == uuid);
    // PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "");

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Uuid).Get<plUuid>() == uuid);
    // PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plAngle)")
  {
    plAngle t = plAngle::Degree(123.0);
    plVariant v(t);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::Angle);

    PLASMA_TEST_BOOL(v.ConvertTo<plAngle>() == t);
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "123.0°");
    // PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("123.0°")); // For some reason the compiler stumbles upon the degree sign, encoding weirdness most likely
    // PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("123.0°"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::Angle).Get<plAngle>() == t);
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "123.0°");
    // PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("123.0°"));
    // PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("123.0°"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plHashedString)")
  {
    plVariant v(plMakeHashedString("78"));

    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Invalid) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Bool));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt8));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt16));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt32));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Int64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::UInt64));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Float));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Double));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Color) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector2I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector3I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Vector4I) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Quaternion) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix3) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Matrix4) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::String));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::StringView));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::DataBuffer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Time) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::Angle) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::ColorGamma) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::HashedString));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TempHashedString));
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantArray) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::VariantDictionary) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedPointer) == false);
    PLASMA_TEST_BOOL(v.CanConvertTo(plVariant::Type::TypedObject) == false);

    plResult ConversionStatus = PLASMA_SUCCESS;
    PLASMA_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_FAILURE);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plInt8>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plUInt8>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plInt16>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plUInt16>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plInt32>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plUInt32>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plInt64>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_INT(v.ConvertTo<plUInt64>(&ConversionStatus), 78);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 78.0f);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 78.0);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_STRING(v.ConvertTo<plString>(&ConversionStatus), "78");
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<plStringView>(&ConversionStatus) == "78"_plsv);
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);

    ConversionStatus = PLASMA_FAILURE;
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>(&ConversionStatus) == plTempHashedString("78"));
    PLASMA_TEST_BOOL(ConversionStatus == PLASMA_SUCCESS);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plTempHashedString)")
  {
    plTempHashedString s("VVVV");
    plVariant v(s);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::TempHashedString);

    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("VVVV"));
    PLASMA_TEST_BOOL(v.ConvertTo<plString>() == "0x69d489c8b7fa5f47");

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("VVVV"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::String).Get<plString>() == "0x69d489c8b7fa5f47");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (VariantArray)")
  {
    plVariantArray va;
    va.PushBack(2.5);
    va.PushBack("ABC");
    va.PushBack(plVariant());
    plVariant v(va);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::VariantArray);

    PLASMA_TEST_BOOL(v.ConvertTo<plVariantArray>() == va);
    PLASMA_TEST_STRING(v.ConvertTo<plString>(), "[2.5, ABC, <Invalid>]");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("[2.5, ABC, <Invalid>]"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("[2.5, ABC, <Invalid>]"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::VariantArray).Get<plVariantArray>() == va);
    PLASMA_TEST_STRING(v.ConvertTo(plVariant::Type::String).Get<plString>(), "[2.5, ABC, <Invalid>]");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("[2.5, ABC, <Invalid>]"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("[2.5, ABC, <Invalid>]"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "(Can)ConvertTo (plVariantDictionary)")
  {
    plVariantDictionary va;
    va.Insert("A", 2.5);
    va.Insert("B", "ABC");
    va.Insert("C", plVariant());
    plVariant v(va);

    TestCanOnlyConvertToStringAndID(v, plVariant::Type::VariantDictionary);

    PLASMA_TEST_BOOL(v.ConvertTo<plVariantDictionary>() == va);
    PLASMA_TEST_STRING(v.ConvertTo<plString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    PLASMA_TEST_BOOL(v.ConvertTo<plHashedString>() == plMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    PLASMA_TEST_BOOL(v.ConvertTo<plTempHashedString>() == plTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));

    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::VariantDictionary).Get<plVariantDictionary>() == va);
    PLASMA_TEST_STRING(v.ConvertTo(plVariant::Type::String).Get<plString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::HashedString).Get<plHashedString>() == plMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    PLASMA_TEST_BOOL(v.ConvertTo(plVariant::Type::TempHashedString).Get<plTempHashedString>() == plTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
  }
}

#pragma optimize("", on)
