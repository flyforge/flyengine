#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

struct FunctionTest
{
  int StandardTypeFunction(int v, const plVec2 vCv, plVec3& ref_vRv, const plVec4& vCrv, plVec2U32* pPv, const plVec3U32* pCpv)
  {
    PLASMA_TEST_BOOL(m_values[0] == v);
    PLASMA_TEST_BOOL(m_values[1] == vCv);
    PLASMA_TEST_BOOL(m_values[2] == ref_vRv);
    PLASMA_TEST_BOOL(m_values[3] == vCrv);
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPv);
      PLASMA_TEST_BOOL(!pCpv);
    }
    else
    {
      PLASMA_TEST_BOOL(m_values[4] == *pPv);
      PLASMA_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_vRv.Set(1, 2, 3);
    if (pPv)
    {
      pPv->Set(1, 2);
    }
    return 5;
  }

  plVarianceTypeAngle CustomTypeFunction(plVarianceTypeAngle v, const plVarianceTypeAngle cv, plVarianceTypeAngle& ref_rv, const plVarianceTypeAngle& crv, plVarianceTypeAngle* pPv, const plVarianceTypeAngle* pCpv)
  {
    PLASMA_TEST_BOOL(m_values[0] == v);
    PLASMA_TEST_BOOL(m_values[1] == cv);
    PLASMA_TEST_BOOL(m_values[2] == ref_rv);
    PLASMA_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPv);
      PLASMA_TEST_BOOL(!pCpv);
    }
    else
    {
      PLASMA_TEST_BOOL(m_values[4] == *pPv);
      PLASMA_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = {2.0f, plAngle::Degree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, plAngle::Degree(400.0f)};
    }
    return {0.6f, plAngle::Degree(60.0f)};
  }

  plVarianceTypeAngle CustomTypeFunction2(plVarianceTypeAngle v, const plVarianceTypeAngle cv, plVarianceTypeAngle& ref_rv, const plVarianceTypeAngle& crv, plVarianceTypeAngle* pPv, const plVarianceTypeAngle* pCpv)
  {
    PLASMA_TEST_BOOL(*m_values[0].Get<plVarianceTypeAngle*>() == v);
    PLASMA_TEST_BOOL(*m_values[1].Get<plVarianceTypeAngle*>() == cv);
    PLASMA_TEST_BOOL(*m_values[2].Get<plVarianceTypeAngle*>() == ref_rv);
    PLASMA_TEST_BOOL(*m_values[3].Get<plVarianceTypeAngle*>() == crv);
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPv);
      PLASMA_TEST_BOOL(!pCpv);
    }
    else
    {
      PLASMA_TEST_BOOL(*m_values[4].Get<plVarianceTypeAngle*>() == *pPv);
      PLASMA_TEST_BOOL(*m_values[5].Get<plVarianceTypeAngle*>() == *pCpv);
    }
    ref_rv = {2.0f, plAngle::Degree(200.0f)};
    if (pPv)
    {
      *pPv = {4.0f, plAngle::Degree(400.0f)};
    }
    return {0.6f, plAngle::Degree(60.0f)};
  }

  const char* StringTypeFunction(const char* szString, plString& ref_sString, plStringView sView)
  {
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!szString);
    }
    else
    {
      PLASMA_TEST_BOOL(m_values[0] == szString);
    }
    PLASMA_TEST_BOOL(m_values[1] == ref_sString);
    PLASMA_TEST_BOOL(m_values[2] == sView);
    return "StringRet";
  }

  plEnum<plExampleEnum> EnumFunction(
    plEnum<plExampleEnum> e, plEnum<plExampleEnum>& ref_re, const plEnum<plExampleEnum>& cre, plEnum<plExampleEnum>* pPe, const plEnum<plExampleEnum>* pCpe)
  {
    PLASMA_TEST_BOOL(m_values[0].Get<plInt64>() == e.GetValue());
    PLASMA_TEST_BOOL(m_values[1].Get<plInt64>() == ref_re.GetValue());
    PLASMA_TEST_BOOL(m_values[2].Get<plInt64>() == cre.GetValue());
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPe);
      PLASMA_TEST_BOOL(!pCpe);
    }
    else
    {
      PLASMA_TEST_BOOL(m_values[3].Get<plInt64>() == pPe->GetValue());
      PLASMA_TEST_BOOL(m_values[4].Get<plInt64>() == pCpe->GetValue());
    }
    return plExampleEnum::Value1;
  }

  plBitflags<plExampleBitflags> BitflagsFunction(plBitflags<plExampleBitflags> e, plBitflags<plExampleBitflags>& ref_re,
    const plBitflags<plExampleBitflags>& cre, plBitflags<plExampleBitflags>* pPe, const plBitflags<plExampleBitflags>* pCpe)
  {
    PLASMA_TEST_BOOL(e == m_values[0].Get<plInt64>());
    PLASMA_TEST_BOOL(ref_re == m_values[1].Get<plInt64>());
    PLASMA_TEST_BOOL(cre == m_values[2].Get<plInt64>());
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPe);
      PLASMA_TEST_BOOL(!pCpe);
    }
    else
    {
      PLASMA_TEST_BOOL(*pPe == m_values[3].Get<plInt64>());
      PLASMA_TEST_BOOL(*pCpe == m_values[4].Get<plInt64>());
    }
    return plExampleBitflags::Value1 | plExampleBitflags::Value2;
  }

  plTestStruct3 StructFunction(
    plTestStruct3 s, const plTestStruct3 cs, plTestStruct3& ref_rs, const plTestStruct3& crs, plTestStruct3* pPs, const plTestStruct3* pCps)
  {
    PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[0].Get<void*>()) == s);
    PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[1].Get<void*>()) == cs);
    PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[2].Get<void*>()) == ref_rs);
    PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[3].Get<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPs);
      PLASMA_TEST_BOOL(!pCps);
    }
    else
    {
      PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[4].Get<void*>()) == *pPs);
      PLASMA_TEST_BOOL(*static_cast<plTestStruct3*>(m_values[5].Get<void*>()) == *pCps);
    }
    ref_rs.m_fFloat1 = 999.0f;
    ref_rs.m_UInt8 = 666;
    if (pPs)
    {
      pPs->m_fFloat1 = 666.0f;
      pPs->m_UInt8 = 999;
    }
    plTestStruct3 retS;
    retS.m_fFloat1 = 42;
    retS.m_UInt8 = 42;
    return retS;
  }

  plTestClass1 ReflectedClassFunction(
    plTestClass1 s, const plTestClass1 cs, plTestClass1& ref_rs, const plTestClass1& crs, plTestClass1* pPs, const plTestClass1* pCps)
  {
    PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[0].ConvertTo<void*>()) == s);
    PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[1].ConvertTo<void*>()) == cs);
    PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[2].ConvertTo<void*>()) == ref_rs);
    PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[3].ConvertTo<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      PLASMA_TEST_BOOL(!pPs);
      PLASMA_TEST_BOOL(!pCps);
    }
    else
    {
      PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[4].ConvertTo<void*>()) == *pPs);
      PLASMA_TEST_BOOL(*static_cast<plTestClass1*>(m_values[5].ConvertTo<void*>()) == *pCps);
    }
    ref_rs.m_Color.SetRGB(1, 2, 3);
    ref_rs.m_MyVector.Set(1, 2, 3);
    if (pPs)
    {
      pPs->m_Color.SetRGB(1, 2, 3);
      pPs->m_MyVector.Set(1, 2, 3);
    }
    plTestClass1 retS;
    retS.m_Color.SetRGB(42, 42, 42);
    retS.m_MyVector.Set(42, 42, 42);
    return retS;
  }

  plVariant VariantFunction(plVariant v, const plVariant cv, plVariant& ref_rv, const plVariant& crv, plVariant* pPv, const plVariant* pCpv)
  {
    PLASMA_TEST_BOOL(m_values[0] == v);
    PLASMA_TEST_BOOL(m_values[1] == cv);
    PLASMA_TEST_BOOL(m_values[2] == ref_rv);
    PLASMA_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      // Can't have variant as nullptr as it must exist in the array and there is no further
      // way of distinguishing a between a plVariant* and a plVariant that is invalid.
      PLASMA_TEST_BOOL(!pPv->IsValid());
      PLASMA_TEST_BOOL(!pCpv->IsValid());
    }
    else
    {
      PLASMA_TEST_BOOL(m_values[4] == *pPv);
      PLASMA_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = plVec3(1, 2, 3);
    if (pPv)
    {
      *pPv = plVec2U32(1, 2);
    }
    return 5;
  }

  static void StaticFunction(bool b, plVariant v)
  {
    PLASMA_TEST_BOOL(b == true);
    PLASMA_TEST_BOOL(v == 4.0f);
  }

  static int StaticFunction2() { return 42; }

  bool m_bPtrAreNull = false;
  plDynamicArray<plVariant> m_values;
};

using ParamSig = std::tuple<const plRTTI*, plBitflags<plPropertyFlags>>;

void VerifyFunctionSignature(const plAbstractFunctionProperty* pFunc, plArrayPtr<ParamSig> params, ParamSig ret)
{
  PLASMA_TEST_INT(params.GetCount(), pFunc->GetArgumentCount());
  for (plUInt32 i = 0; i < plMath::Min(params.GetCount(), pFunc->GetArgumentCount()); i++)
  {
    PLASMA_TEST_BOOL(pFunc->GetArgumentType(i) == std::get<0>(params[i]));
    PLASMA_TEST_BOOL(pFunc->GetArgumentFlags(i) == std::get<1>(params[i]));
  }
  PLASMA_TEST_BOOL(pFunc->GetReturnType() == std::get<0>(ret));
  PLASMA_TEST_BOOL(pFunc->GetReturnFlags() == std::get<1>(ret));
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, Functions)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - StandardTypes")
  {
    plFunctionProperty<decltype(&FunctionTest::StandardTypeFunction)> funccall("", &FunctionTest::StandardTypeFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<int>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plVec2>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plVec3>(), plPropertyFlags::StandardType | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVec4>(), plPropertyFlags::StandardType | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVec2U32>(), plPropertyFlags::StandardType | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plVec3U32>(), plPropertyFlags::StandardType | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<int>(), plPropertyFlags::StandardType));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(plVec2(2));
    test.m_values.PushBack(plVec3(3));
    test.m_values.PushBack(plVec4(4));
    test.m_values.PushBack(plVec2U32(5));
    test.m_values.PushBack(plVec3U32(6));

    plVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int32);
    PLASMA_TEST_BOOL(ret == 5);
    PLASMA_TEST_BOOL(test.m_values[2] == plVec3(1, 2, 3));
    PLASMA_TEST_BOOL(test.m_values[4] == plVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = plVariant();
    test.m_values[5] = plVariant();
    ret = plVariant();
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int32);
    PLASMA_TEST_BOOL(ret == 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - CustomType")
  {
    plFunctionProperty<decltype(&FunctionTest::CustomTypeFunction)> funccall("", &FunctionTest::CustomTypeFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plVarianceTypeAngle>(), plPropertyFlags::Class));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    {
      FunctionTest test;
      test.m_values.PushBack(plVarianceTypeAngle{0.0f, plAngle::Degree(0.0f)});
      test.m_values.PushBack(plVarianceTypeAngle{0.1f, plAngle::Degree(10.0f)});
      test.m_values.PushBack(plVarianceTypeAngle{0.2f, plAngle::Degree(20.0f)});
      test.m_values.PushBack(plVarianceTypeAngle{0.3f, plAngle::Degree(30.0f)});
      test.m_values.PushBack(plVarianceTypeAngle{0.4f, plAngle::Degree(40.0f)});
      test.m_values.PushBack(plVarianceTypeAngle{0.5f, plAngle::Degree(50.0f)});

      plVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedObject);
      PLASMA_TEST_BOOL(ret == plVariant(plVarianceTypeAngle{0.6f, plAngle::Degree(60.0f)}));
      PLASMA_TEST_BOOL(test.m_values[2] == plVariant(plVarianceTypeAngle{2.0f, plAngle::Degree(200.0f)}));
      PLASMA_TEST_BOOL(test.m_values[4] == plVariant(plVarianceTypeAngle{4.0f, plAngle::Degree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4] = plVariant();
      test.m_values[5] = plVariant();
      ret = plVariant();
      funccall.Execute(&test, test.m_values, ret);
      PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedObject);
      PLASMA_TEST_BOOL(ret == plVariant(plVarianceTypeAngle{0.6f, plAngle::Degree(60.0f)}));
    }

    {
      plFunctionProperty<decltype(&FunctionTest::CustomTypeFunction2)> funccall2("", &FunctionTest::CustomTypeFunction2);

      FunctionTest test;
      plVarianceTypeAngle v0{0.0f, plAngle::Degree(0.0f)};
      plVarianceTypeAngle v1{0.1f, plAngle::Degree(10.0f)};
      plVarianceTypeAngle v2{0.2f, plAngle::Degree(20.0f)};
      plVarianceTypeAngle v3{0.3f, plAngle::Degree(30.0f)};
      plVarianceTypeAngle v4{0.4f, plAngle::Degree(40.0f)};
      plVarianceTypeAngle v5{0.5f, plAngle::Degree(50.0f)};
      test.m_values.PushBack(&v0);
      test.m_values.PushBack(&v1);
      test.m_values.PushBack(&v2);
      test.m_values.PushBack(&v3);
      test.m_values.PushBack(&v4);
      test.m_values.PushBack(&v5);

      plVariant ret;
      funccall2.Execute(&test, test.m_values, ret);
      PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedObject);
      PLASMA_TEST_BOOL(ret == plVariant(plVarianceTypeAngle{0.6f, plAngle::Degree(60.0f)}));
      PLASMA_TEST_BOOL((*test.m_values[2].Get<plVarianceTypeAngle*>() == plVarianceTypeAngle{2.0f, plAngle::Degree(200.0f)}));
      PLASMA_TEST_BOOL((*test.m_values[4].Get<plVarianceTypeAngle*>() == plVarianceTypeAngle{4.0f, plAngle::Degree(400.0f)}));

      test.m_bPtrAreNull = true;
      test.m_values[4] = plVariant();
      test.m_values[5] = plVariant();
      ret = plVariant();
      funccall2.Execute(&test, test.m_values, ret);
      PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedObject);
      PLASMA_TEST_BOOL(ret == plVariant(plVarianceTypeAngle{0.6f, plAngle::Degree(60.0f)}));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Strings")
  {
    plFunctionProperty<decltype(&FunctionTest::StringTypeFunction)> funccall("", &FunctionTest::StringTypeFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<const char*>(), plPropertyFlags::StandardType | plPropertyFlags::Const),
      ParamSig(plGetStaticRTTI<plString>(), plPropertyFlags::StandardType | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plStringView>(), plPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<const char*>(), plPropertyFlags::StandardType | plPropertyFlags::Const));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(plVariant(plString("String0")));
    test.m_values.PushBack(plVariant(plString("String1")));
    test.m_values.PushBack(plVariant(plStringView("String2"), false));

    plVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::String);
    PLASMA_TEST_BOOL(ret == plString("StringRet"));

    test.m_bPtrAreNull = true;
    test.m_values[0] = plVariant();
    ret = plVariant();
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::String);
    PLASMA_TEST_BOOL(ret == plString("StringRet"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Enum")
  {
    plFunctionProperty<decltype(&FunctionTest::EnumFunction)> funccall("", &FunctionTest::EnumFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum),
      ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plExampleEnum>(), plPropertyFlags::IsEnum));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((plInt64)plExampleEnum::Value1);
    test.m_values.PushBack((plInt64)plExampleEnum::Value2);
    test.m_values.PushBack((plInt64)plExampleEnum::Value3);
    test.m_values.PushBack((plInt64)plExampleEnum::Default);
    test.m_values.PushBack((plInt64)plExampleEnum::Value3);

    plVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int64);
    PLASMA_TEST_BOOL(ret == (plInt64)plExampleEnum::Value1);

    test.m_bPtrAreNull = true;
    test.m_values[3] = plVariant();
    test.m_values[4] = plVariant();
    ret = plVariant();
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int64);
    PLASMA_TEST_BOOL(ret == (plInt64)plExampleEnum::Value1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Bitflags")
  {
    plFunctionProperty<decltype(&FunctionTest::BitflagsFunction)> funccall("", &FunctionTest::BitflagsFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags),
      ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plExampleBitflags>(), plPropertyFlags::Bitflags));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((plInt64)(0));
    test.m_values.PushBack((plInt64)(plExampleBitflags::Value2));
    test.m_values.PushBack((plInt64)(plExampleBitflags::Value3 | plExampleBitflags::Value2).GetValue());
    test.m_values.PushBack((plInt64)(plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3).GetValue());
    test.m_values.PushBack((plInt64)(plExampleBitflags::Value3));

    plVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int64);
    PLASMA_TEST_BOOL(ret == (plInt64)(plExampleBitflags::Value1 | plExampleBitflags::Value2).GetValue());

    test.m_bPtrAreNull = true;
    test.m_values[3] = plVariant();
    test.m_values[4] = plVariant();
    ret = plVariant();
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int64);
    PLASMA_TEST_BOOL(ret == (plInt64)(plExampleBitflags::Value1 | plExampleBitflags::Value2).GetValue());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Structs")
  {
    plFunctionProperty<decltype(&FunctionTest::StructFunction)> funccall("", &FunctionTest::StructFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    plTestStruct3 retS;
    retS.m_fFloat1 = 0;
    retS.m_UInt8 = 0;
    plTestStruct3 value;
    value.m_fFloat1 = 0;
    value.m_UInt8 = 0;
    plTestStruct3 rs;
    rs.m_fFloat1 = 42;
    plTestStruct3 ps;
    ps.m_fFloat1 = 18;

    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&rs));
    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&ps));
    test.m_values.PushBack(plVariant(&value));

    // plVariantAdapter<plTestStruct3 const*> aa(plVariant(&value));
    // auto bla = plIsStandardType<plTestStruct3 const*>::value;

    plVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_FLOAT(retS.m_fFloat1, 42, 0);
    PLASMA_TEST_INT(retS.m_UInt8, 42);

    PLASMA_TEST_FLOAT(rs.m_fFloat1, 999, 0);
    PLASMA_TEST_INT(rs.m_UInt8, 666);

    PLASMA_TEST_DOUBLE(ps.m_fFloat1, 666, 0);
    PLASMA_TEST_INT(ps.m_UInt8, 999);

    test.m_bPtrAreNull = true;
    test.m_values[4] = plVariant();
    test.m_values[5] = plVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Reflected Classes")
  {
    plFunctionProperty<decltype(&FunctionTest::ReflectedClassFunction)> funccall("", &FunctionTest::ReflectedClassFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class),
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    plTestClass1 retS;
    retS.m_Color = plColor::Chocolate;
    plTestClass1 value;
    value.m_Color = plColor::AliceBlue;
    plTestClass1 rs;
    rs.m_Color = plColor::Beige;
    plTestClass1 ps;
    ps.m_Color = plColor::DarkBlue;

    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&rs));
    test.m_values.PushBack(plVariant(&value));
    test.m_values.PushBack(plVariant(&ps));
    test.m_values.PushBack(plVariant(&value));

    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);


    plVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(retS.m_Color == plColor(42, 42, 42));
    PLASMA_TEST_BOOL(retS.m_MyVector == plVec3(42, 42, 42));

    PLASMA_TEST_BOOL(rs.m_Color == plColor(1, 2, 3));
    PLASMA_TEST_BOOL(rs.m_MyVector == plVec3(1, 2, 3));

    PLASMA_TEST_BOOL(ps.m_Color == plColor(1, 2, 3));
    PLASMA_TEST_BOOL(ps.m_MyVector == plVec3(1, 2, 3));

    test.m_bPtrAreNull = true;
    test.m_values[4] = plVariant();
    test.m_values[5] = plVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Functions - Variant")
  {
    plFunctionProperty<decltype(&FunctionTest::VariantFunction)> funccall("", &FunctionTest::VariantFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType | plPropertyFlags::Pointer),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType | plPropertyFlags::Const | plPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(plVec2(2));
    test.m_values.PushBack(plVec3(3));
    test.m_values.PushBack(plVec4(4));
    test.m_values.PushBack(plVec2U32(5));
    test.m_values.PushBack(plVec3U32(6));

    plVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int32);
    PLASMA_TEST_BOOL(ret == 5);
    PLASMA_TEST_BOOL(test.m_values[2] == plVec3(1, 2, 3));
    PLASMA_TEST_BOOL(test.m_values[4] == plVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = plVariant();
    test.m_values[5] = plVariant();
    ret = plVariant();
    funccall.Execute(&test, test.m_values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int32);
    PLASMA_TEST_BOOL(ret == 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Static Functions")
  {
    // Void return
    plFunctionProperty<decltype(&FunctionTest::StaticFunction)> funccall("", &FunctionTest::StaticFunction);
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<bool>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plVariant>(), plPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<void>(), plPropertyFlags::Void));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::StaticMember);

    plDynamicArray<plVariant> values;
    values.PushBack(true);
    values.PushBack(4.0f);
    plVariant ret;
    funccall.Execute(nullptr, values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Invalid);

    // Zero parameter
    plFunctionProperty<decltype(&FunctionTest::StaticFunction2)> funccall2("", &FunctionTest::StaticFunction2);
    VerifyFunctionSignature(&funccall2, plArrayPtr<ParamSig>(), ParamSig(plGetStaticRTTI<int>(), plPropertyFlags::StandardType));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::StaticMember);
    values.Clear();
    funccall2.Execute(nullptr, values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Int32);
    PLASMA_TEST_BOOL(ret == 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor Functions - StandardTypes")
  {
    plConstructorFunctionProperty<plVec4, float, float, float, float> funccall;
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<float>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<float>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<float>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<float>(), plPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plVec4>(), plPropertyFlags::StandardType | plPropertyFlags::Pointer));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Constructor);

    plDynamicArray<plVariant> values;
    values.PushBack(1.0f);
    values.PushBack(2.0f);
    values.PushBack(3.0f);
    values.PushBack(4.0f);
    plVariant ret;
    funccall.Execute(nullptr, values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::Vector4);
    PLASMA_TEST_BOOL(ret == plVec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor Functions - Struct")
  {
    plConstructorFunctionProperty<plTestStruct3, double, plInt16> funccall;
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<double>(), plPropertyFlags::StandardType),
      ParamSig(plGetStaticRTTI<plInt16>(), plPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plTestStruct3>(), plPropertyFlags::Class | plPropertyFlags::Pointer));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Constructor);

    plDynamicArray<plVariant> values;
    values.PushBack(59.0);
    values.PushBack((plInt16)666);
    plVariant ret;
    funccall.Execute(nullptr, values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedPointer);
    plTestStruct3* pRet = static_cast<plTestStruct3*>(ret.ConvertTo<void*>());
    PLASMA_TEST_BOOL(pRet != nullptr);

    PLASMA_TEST_FLOAT(pRet->m_fFloat1, 59.0, 0);
    PLASMA_TEST_INT(pRet->m_UInt8, 666);
    PLASMA_TEST_INT(pRet->GetIntPublic(), 32);

    PLASMA_DEFAULT_DELETE(pRet);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor Functions - Reflected Classes")
  {
    // The function signature does not actually need to match the ctor 100% as long as implicit conversion is possible.
    plConstructorFunctionProperty<plTestClass1, const plColor&, const plTestStruct&> funccall;
    ParamSig testSet[] = {
      ParamSig(plGetStaticRTTI<plColor>(), plPropertyFlags::StandardType | plPropertyFlags::Const | plPropertyFlags::Reference),
      ParamSig(plGetStaticRTTI<plTestStruct>(), plPropertyFlags::Class | plPropertyFlags::Const | plPropertyFlags::Reference),
    };
    VerifyFunctionSignature(
      &funccall, plArrayPtr<ParamSig>(testSet), ParamSig(plGetStaticRTTI<plTestClass1>(), plPropertyFlags::Class | plPropertyFlags::Pointer));
    PLASMA_TEST_BOOL(funccall.GetFunctionType() == plFunctionType::Constructor);

    plDynamicArray<plVariant> values;
    plTestStruct s;
    s.m_fFloat1 = 1.0f;
    s.m_UInt8 = 255;
    values.PushBack(plColor::CornflowerBlue);
    values.PushBack(plVariant(&s));
    plVariant ret;
    funccall.Execute(nullptr, values, ret);
    PLASMA_TEST_BOOL(ret.GetType() == plVariantType::TypedPointer);
    plTestClass1* pRet = static_cast<plTestClass1*>(ret.ConvertTo<void*>());
    PLASMA_TEST_BOOL(pRet != nullptr);

    PLASMA_TEST_BOOL(pRet->m_Color == plColor::CornflowerBlue);
    PLASMA_TEST_BOOL(pRet->m_Struct == s);
    PLASMA_TEST_BOOL(pRet->m_MyVector == plVec3(1, 2, 3));

    PLASMA_DEFAULT_DELETE(pRet);
  }
}
