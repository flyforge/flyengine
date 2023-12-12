#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>


template <typename T>
void TestSerialization(const T& source)
{
  plDefaultMemoryStreamStorage StreamStorage;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "WriteObjectToDDL")
  {
    plMemoryStreamWriter FileOut(&StreamStorage);

    plReflectionSerializer::WriteObjectToDDL(FileOut, plGetStaticRTTI<T>(), &source, false, plOpenDdlWriter::TypeStringMode::Compliant);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    plMemoryStreamReader FileIn(&StreamStorage);
    T data;
    plReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *plGetStaticRTTI<T>(), &data);

    PLASMA_TEST_BOOL(data == source);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectFromDDL")
  {
    plMemoryStreamReader FileIn(&StreamStorage);

    const plRTTI* pRtti;
    void* pObject = plReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    PLASMA_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  plDefaultMemoryStreamStorage StreamStorageBinary;
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "WriteObjectToBinary")
  {
    plMemoryStreamWriter FileOut(&StreamStorageBinary);

    plReflectionSerializer::WriteObjectToBinary(FileOut, plGetStaticRTTI<T>(), &source);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectPropertiesFromBinary")
  {
    plMemoryStreamReader FileIn(&StreamStorageBinary);
    T data;
    plReflectionSerializer::ReadObjectPropertiesFromBinary(FileIn, *plGetStaticRTTI<T>(), &data);

    PLASMA_TEST_BOOL(data == source);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadObjectFromBinary")
  {
    plMemoryStreamReader FileIn(&StreamStorageBinary);

    const plRTTI* pRtti;
    void* pObject = plReflectionSerializer::ReadObjectFromBinary(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    PLASMA_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clone")
  {
    {
      T clone;
      plReflectionSerializer::Clone(&source, &clone, plGetStaticRTTI<T>());
      PLASMA_TEST_BOOL(clone == source);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(&clone, &source, plGetStaticRTTI<T>()));
    }

    {
      T* pClone = plReflectionSerializer::Clone(&source);
      PLASMA_TEST_BOOL(*pClone == source);
      PLASMA_TEST_BOOL(plReflectionUtils::IsEqual(pClone, &source));
      plGetStaticRTTI<T>()->GetAllocator()->Deallocate(pClone);
    }
  }
}


PLASMA_CREATE_SIMPLE_TEST_GROUP(Reflection);

PLASMA_CREATE_SIMPLE_TEST(Reflection, Types)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Iterate All")
  {
    bool bFoundStruct = false;
    bool bFoundClass1 = false;
    bool bFoundClass2 = false;

    plRTTI::ForEachType([&](const plRTTI* pRtti) {
        if (pRtti->GetTypeName() == "plTestStruct")
          bFoundStruct = true;
        if (pRtti->GetTypeName() == "plTestClass1")
          bFoundClass1 = true;
        if (pRtti->GetTypeName() == "plTestClass2")
          bFoundClass2 = true;

        PLASMA_TEST_STRING(pRtti->GetPluginName(), "Static"); });

    PLASMA_TEST_BOOL(bFoundStruct);
    PLASMA_TEST_BOOL(bFoundClass1);
    PLASMA_TEST_BOOL(bFoundClass2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsDerivedFrom")
  {
    plDynamicArray<const plRTTI*> allTypes;
    plRTTI::ForEachType([&](const plRTTI* pRtti) { allTypes.PushBack(pRtti); });

    // ground truth - traversing up the parent list
    auto ManualIsDerivedFrom = [](const plRTTI* t, const plRTTI* pBaseType) -> bool {
      while (t != nullptr)
      {
        if (t == pBaseType)
          return true;

        t = t->GetParentType();
      }

      return false;
    };

    // test each type against every other:
    for (const plRTTI* typeA : allTypes)
    {
      for (const plRTTI* typeB : allTypes)
      {
        bool derived = typeA->IsDerivedFrom(typeB);
        bool manualCheck = ManualIsDerivedFrom(typeA, typeB);
        PLASMA_TEST_BOOL(derived == manualCheck);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PropertyFlags")
  {
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<void>() == (plPropertyFlags::Void));
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<const char*>() == (plPropertyFlags::StandardType | plPropertyFlags::Const));
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<int>() == plPropertyFlags::StandardType);
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<int&>() == (plPropertyFlags::StandardType | plPropertyFlags::Reference));
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<int*>() == (plPropertyFlags::StandardType | plPropertyFlags::Pointer));

    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<const int>() == (plPropertyFlags::StandardType | plPropertyFlags::Const));
    PLASMA_TEST_BOOL(
      plPropertyFlags::GetParameterFlags<const int&>() == (plPropertyFlags::StandardType | plPropertyFlags::Reference | plPropertyFlags::Const));
    PLASMA_TEST_BOOL(
      plPropertyFlags::GetParameterFlags<const int*>() == (plPropertyFlags::StandardType | plPropertyFlags::Pointer | plPropertyFlags::Const));

    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plVariant>() == (plPropertyFlags::StandardType));

    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plExampleEnum::Enum>() == plPropertyFlags::IsEnum);
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plEnum<plExampleEnum>>() == plPropertyFlags::IsEnum);
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plBitflags<plExampleBitflags>>() == plPropertyFlags::Bitflags);

    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plTestStruct3>() == plPropertyFlags::Class);
    PLASMA_TEST_BOOL(plPropertyFlags::GetParameterFlags<plTestClass2>() == plPropertyFlags::Class);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TypeFlags")
  {
    PLASMA_TEST_INT(plGetStaticRTTI<bool>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);
    PLASMA_TEST_INT(plGetStaticRTTI<plUuid>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);
    PLASMA_TEST_INT(plGetStaticRTTI<const char*>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);
    PLASMA_TEST_INT(plGetStaticRTTI<plString>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);
    PLASMA_TEST_INT(plGetStaticRTTI<plMat4>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);
    PLASMA_TEST_INT(plGetStaticRTTI<plVariant>()->GetTypeFlags().GetValue(), plTypeFlags::StandardType);

    PLASMA_TEST_INT(plGetStaticRTTI<plAbstractTestClass>()->GetTypeFlags().GetValue(), (plTypeFlags::Class | plTypeFlags::Abstract).GetValue());
    PLASMA_TEST_INT(plGetStaticRTTI<plAbstractTestStruct>()->GetTypeFlags().GetValue(), (plTypeFlags::Class | plTypeFlags::Abstract).GetValue());

    PLASMA_TEST_INT(plGetStaticRTTI<plTestStruct3>()->GetTypeFlags().GetValue(), plTypeFlags::Class);
    PLASMA_TEST_INT(plGetStaticRTTI<plTestClass2>()->GetTypeFlags().GetValue(), plTypeFlags::Class);

    PLASMA_TEST_INT(plGetStaticRTTI<plExampleEnum>()->GetTypeFlags().GetValue(), plTypeFlags::IsEnum);
    PLASMA_TEST_INT(plGetStaticRTTI<plExampleBitflags>()->GetTypeFlags().GetValue(), plTypeFlags::Bitflags);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindTypeByName")
  {
    const plRTTI* pFloat = plRTTI::FindTypeByName("float");
    PLASMA_TEST_BOOL(pFloat != nullptr);
    PLASMA_TEST_STRING(pFloat->GetTypeName(), "float");

    const plRTTI* pStruct = plRTTI::FindTypeByName("plTestStruct");
    PLASMA_TEST_BOOL(pStruct != nullptr);
    PLASMA_TEST_STRING(pStruct->GetTypeName(), "plTestStruct");

    const plRTTI* pClass2 = plRTTI::FindTypeByName("plTestClass2");
    PLASMA_TEST_BOOL(pClass2 != nullptr);
    PLASMA_TEST_STRING(pClass2->GetTypeName(), "plTestClass2");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindTypeByNameHash")
  {
    const plRTTI* pFloat = plRTTI::FindTypeByName("float");
    const plRTTI* pFloat2 = plRTTI::FindTypeByNameHash(pFloat->GetTypeNameHash());
    PLASMA_TEST_BOOL(pFloat == pFloat2);

    const plRTTI* pStruct = plRTTI::FindTypeByName("plTestStruct");
    const plRTTI* pStruct2 = plRTTI::FindTypeByNameHash(pStruct->GetTypeNameHash());
    PLASMA_TEST_BOOL(pStruct == pStruct2);

    const plRTTI* pClass = plRTTI::FindTypeByName("plTestClass2");
    const plRTTI* pClass2 = plRTTI::FindTypeByNameHash(pClass->GetTypeNameHash());
    PLASMA_TEST_BOOL(pClass == pClass2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetProperties")
  {
    {
      const plRTTI* pType = plRTTI::FindTypeByName("plTestStruct");

      auto Props = pType->GetProperties();
      PLASMA_TEST_INT(Props.GetCount(), 9);
      PLASMA_TEST_STRING(Props[0]->GetPropertyName(), "Float");
      PLASMA_TEST_STRING(Props[1]->GetPropertyName(), "Vector");
      PLASMA_TEST_STRING(Props[2]->GetPropertyName(), "Int");
      PLASMA_TEST_STRING(Props[3]->GetPropertyName(), "UInt8");
      PLASMA_TEST_STRING(Props[4]->GetPropertyName(), "Variant");
      PLASMA_TEST_STRING(Props[5]->GetPropertyName(), "Angle");
      PLASMA_TEST_STRING(Props[6]->GetPropertyName(), "DataBuffer");
      PLASMA_TEST_STRING(Props[7]->GetPropertyName(), "vVec3I");
      PLASMA_TEST_STRING(Props[8]->GetPropertyName(), "VarianceAngle");
    }

    {
      const plRTTI* pType = plRTTI::FindTypeByName("plTestClass2");

      auto Props = pType->GetProperties();
      PLASMA_TEST_INT(Props.GetCount(), 6);
      PLASMA_TEST_STRING(Props[0]->GetPropertyName(), "Text");
      PLASMA_TEST_STRING(Props[1]->GetPropertyName(), "Time");
      PLASMA_TEST_STRING(Props[2]->GetPropertyName(), "Enum");
      PLASMA_TEST_STRING(Props[3]->GetPropertyName(), "Bitflags");
      PLASMA_TEST_STRING(Props[4]->GetPropertyName(), "Array");
      PLASMA_TEST_STRING(Props[5]->GetPropertyName(), "Variant");

      plHybridArray<const plAbstractProperty*, 32> AllProps;
      pType->GetAllProperties(AllProps);

      PLASMA_TEST_INT(AllProps.GetCount(), 9);
      PLASMA_TEST_STRING(AllProps[0]->GetPropertyName(), "SubStruct");
      PLASMA_TEST_STRING(AllProps[1]->GetPropertyName(), "Color");
      PLASMA_TEST_STRING(AllProps[2]->GetPropertyName(), "SubVector");
      PLASMA_TEST_STRING(AllProps[3]->GetPropertyName(), "Text");
      PLASMA_TEST_STRING(AllProps[4]->GetPropertyName(), "Time");
      PLASMA_TEST_STRING(AllProps[5]->GetPropertyName(), "Enum");
      PLASMA_TEST_STRING(AllProps[6]->GetPropertyName(), "Bitflags");
      PLASMA_TEST_STRING(AllProps[7]->GetPropertyName(), "Array");
      PLASMA_TEST_STRING(AllProps[8]->GetPropertyName(), "Variant");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Casts")
  {
    plTestClass2 test;
    plTestClass1* pTestClass1 = &test;
    const plTestClass1* pConstTestClass1 = &test;

    plTestClass2* pTestClass2 = plStaticCast<plTestClass2*>(pTestClass1);
    const plTestClass2* pConstTestClass2 = plStaticCast<const plTestClass2*>(pConstTestClass1);

    pTestClass2 = plDynamicCast<plTestClass2*>(pTestClass1);
    pConstTestClass2 = plDynamicCast<const plTestClass2*>(pConstTestClass1);
    PLASMA_TEST_BOOL(pTestClass2 != nullptr);
    PLASMA_TEST_BOOL(pConstTestClass2 != nullptr);

    plTestClass1 otherTest;
    pTestClass1 = &otherTest;
    pConstTestClass1 = &otherTest;

    pTestClass2 = plDynamicCast<plTestClass2*>(pTestClass1);
    pConstTestClass2 = plDynamicCast<const plTestClass2*>(pConstTestClass1);
    PLASMA_TEST_BOOL(pTestClass2 == nullptr);
    PLASMA_TEST_BOOL(pConstTestClass2 == nullptr);
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Types From Plugin")
  {
    plResult loadPlugin = plPlugin::LoadPlugin(plasmaFoundationTest_Plugin1);
    PLASMA_TEST_BOOL(loadPlugin == PLASMA_SUCCESS);

    if (loadPlugin.Failed())
      return;

    const plRTTI* pStruct2 = plRTTI::FindTypeByName("plTestStruct2");
    PLASMA_TEST_BOOL(pStruct2 != nullptr);

    if (pStruct2)
    {
      PLASMA_TEST_STRING(pStruct2->GetTypeName(), "plTestStruct2");
    }

    bool bFoundStruct2 = false;

    plRTTI::ForEachType(
      [&](const plRTTI* pRtti) {
        if (pRtti->GetTypeName() == "plTestStruct2")
        {
          bFoundStruct2 = true;

          PLASMA_TEST_STRING(pRtti->GetPluginName(), plasmaFoundationTest_Plugin1);

          void* pInstance = pRtti->GetAllocator()->Allocate<void>();
          PLASMA_TEST_BOOL(pInstance != nullptr);

          const plAbstractProperty* pProp = pRtti->FindPropertyByName("Float2");

          PLASMA_TEST_BOOL(pProp != nullptr);

          PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
          plAbstractMemberProperty* pAbsMember = (plAbstractMemberProperty*)pProp;

          PLASMA_TEST_BOOL(pAbsMember->GetSpecificType() == plGetStaticRTTI<float>());

          plTypedMemberProperty<float>* pMember = (plTypedMemberProperty<float>*)pAbsMember;

          PLASMA_TEST_FLOAT(pMember->GetValue(pInstance), 42.0f, 0);
          pMember->SetValue(pInstance, 43.0f);
          PLASMA_TEST_FLOAT(pMember->GetValue(pInstance), 43.0f, 0);

          pRtti->GetAllocator()->Deallocate(pInstance);
        }
        else
        {
          PLASMA_TEST_STRING(pRtti->GetPluginName(), "Static");
        }
      });

    PLASMA_TEST_BOOL(bFoundStruct2);

    plPlugin::UnloadAllPlugins();
  }
#endif
}


PLASMA_CREATE_SIMPLE_TEST(Reflection, Hierarchies)
{
  plTestClass2Allocator::m_iAllocs = 0;
  plTestClass2Allocator::m_iDeallocs = 0;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTestStruct")
  {
    const plRTTI* pRtti = plGetStaticRTTI<plTestStruct>();

    PLASMA_TEST_STRING(pRtti->GetTypeName(), "plTestStruct");
    PLASMA_TEST_INT(pRtti->GetTypeSize(), sizeof(plTestStruct));
    PLASMA_TEST_BOOL(pRtti->GetVariantType() == plVariant::Type::Invalid);

    PLASMA_TEST_BOOL(pRtti->GetParentType() == nullptr);

    PLASMA_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTestClass1")
  {
    const plRTTI* pRtti = plGetStaticRTTI<plTestClass1>();

    PLASMA_TEST_STRING(pRtti->GetTypeName(), "plTestClass1");
    PLASMA_TEST_INT(pRtti->GetTypeSize(), sizeof(plTestClass1));
    PLASMA_TEST_BOOL(pRtti->GetVariantType() == plVariant::Type::Invalid);

    PLASMA_TEST_BOOL(pRtti->GetParentType() == plGetStaticRTTI<plReflectedClass>());

    PLASMA_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    plTestClass1* pInstance = pRtti->GetAllocator()->Allocate<plTestClass1>();
    PLASMA_TEST_BOOL(pInstance != nullptr);

    PLASMA_TEST_BOOL(pInstance->GetDynamicRTTI() == plGetStaticRTTI<plTestClass1>());
    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom<plReflectedClass>());
    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom(plGetStaticRTTI<plReflectedClass>()));

    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom<plTestClass1>());
    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom(plGetStaticRTTI<plTestClass1>()));

    PLASMA_TEST_BOOL(!pRtti->IsDerivedFrom<plVec3>());
    PLASMA_TEST_BOOL(!pRtti->IsDerivedFrom(plGetStaticRTTI<plVec3>()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTestClass2")
  {
    const plRTTI* pRtti = plGetStaticRTTI<plTestClass2>();

    PLASMA_TEST_STRING(pRtti->GetTypeName(), "plTestClass2");
    PLASMA_TEST_INT(pRtti->GetTypeSize(), sizeof(plTestClass2));
    PLASMA_TEST_BOOL(pRtti->GetVariantType() == plVariant::Type::Invalid);

    PLASMA_TEST_BOOL(pRtti->GetParentType() == plGetStaticRTTI<plTestClass1>());

    PLASMA_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    PLASMA_TEST_INT(plTestClass2Allocator::m_iAllocs, 0);
    PLASMA_TEST_INT(plTestClass2Allocator::m_iDeallocs, 0);

    plTestClass2* pInstance = pRtti->GetAllocator()->Allocate<plTestClass2>();
    PLASMA_TEST_BOOL(pInstance != nullptr);

    PLASMA_TEST_BOOL(pInstance->GetDynamicRTTI() == plGetStaticRTTI<plTestClass2>());

    PLASMA_TEST_INT(plTestClass2Allocator::m_iAllocs, 1);
    PLASMA_TEST_INT(plTestClass2Allocator::m_iDeallocs, 0);

    pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

    PLASMA_TEST_INT(plTestClass2Allocator::m_iAllocs, 1);
    PLASMA_TEST_INT(plTestClass2Allocator::m_iDeallocs, 1);

    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom<plTestClass1>());
    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom(plGetStaticRTTI<plTestClass1>()));

    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom<plTestClass2>());
    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom(plGetStaticRTTI<plTestClass2>()));

    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom<plReflectedClass>());
    PLASMA_TEST_BOOL(pRtti->IsDerivedFrom(plGetStaticRTTI<plReflectedClass>()));

    PLASMA_TEST_BOOL(!pRtti->IsDerivedFrom<plVec3>());
    PLASMA_TEST_BOOL(!pRtti->IsDerivedFrom(plGetStaticRTTI<plVec3>()));
  }
}


template <typename T, typename T2>
void TestMemberProperty(const char* szPropName, void* pObject, const plRTTI* pRtti, plBitflags<plPropertyFlags> expectedFlags, T2 expectedValue, T2 testValue, bool bTestDefaultValue = true)
{
  const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  PLASMA_TEST_BOOL(pProp != nullptr);

  PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);

  PLASMA_TEST_BOOL(pProp->GetSpecificType() == plGetStaticRTTI<T>());
  const plTypedMemberProperty<T>* pMember = (plTypedMemberProperty<T>*)pProp;

  PLASMA_TEST_INT(pMember->GetFlags().GetValue(), expectedFlags.GetValue());

  T value = pMember->GetValue(pObject);
  PLASMA_TEST_BOOL(expectedValue == value);

  if (bTestDefaultValue)
  {
    // Default value
    plVariant defaultValue = plReflectionUtils::GetDefaultValue(pProp);
    PLASMA_TEST_BOOL(plVariant(expectedValue) == defaultValue);
  }

  if (!pMember->GetFlags().IsSet(plPropertyFlags::ReadOnly))
  {
    pMember->SetValue(pObject, testValue);

    PLASMA_TEST_BOOL(testValue == pMember->GetValue(pObject));

    plReflectionUtils::SetMemberPropertyValue(pMember, pObject, plVariant(expectedValue));
    plVariant res = plReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    PLASMA_TEST_BOOL(res == plVariant(expectedValue));
    PLASMA_TEST_BOOL(res != plVariant(testValue));

    plReflectionUtils::SetMemberPropertyValue(pMember, pObject, plVariant(testValue));
    res = plReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    PLASMA_TEST_BOOL(res != plVariant(expectedValue));
    PLASMA_TEST_BOOL(res == plVariant(testValue));
  }
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, MemberProperties)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTestStruct")
  {
    plTestStruct data;
    const plRTTI* pRtti = plGetStaticRTTI<plTestStruct>();

    TestMemberProperty<float>("Float", &data, pRtti, plPropertyFlags::StandardType, 1.1f, 5.0f);
    TestMemberProperty<plInt32>("Int", &data, pRtti, plPropertyFlags::StandardType, 2, -8);
    TestMemberProperty<plVec3>("Vector", &data, pRtti, plPropertyFlags::StandardType | plPropertyFlags::ReadOnly, plVec3(3, 4, 5),
      plVec3(0, -1.0f, 3.14f));
    TestMemberProperty<plVariant>("Variant", &data, pRtti, plPropertyFlags::StandardType, plVariant("Test"),
      plVariant(plVec3(0, -1.0f, 3.14f)));
    TestMemberProperty<plAngle>("Angle", &data, pRtti, plPropertyFlags::StandardType, plAngle::Degree(0.5f), plAngle::Degree(1.0f));
    plVarianceTypeAngle expectedVA = {0.5f, plAngle::Degree(90.0f)};
    plVarianceTypeAngle testVA = {0.1f, plAngle::Degree(45.0f)};
    TestMemberProperty<plVarianceTypeAngle>("VarianceAngle", &data, pRtti, plPropertyFlags::Class, expectedVA, testVA);

    plDataBuffer expected;
    expected.PushBack(255);
    expected.PushBack(0);
    expected.PushBack(127);

    plDataBuffer newValue;
    newValue.PushBack(1);
    newValue.PushBack(2);

    TestMemberProperty<plDataBuffer>("DataBuffer", &data, pRtti, plPropertyFlags::StandardType, expected, newValue);
    TestMemberProperty<plVec3I32>("vVec3I", &data, pRtti, plPropertyFlags::StandardType, plVec3I32(1, 2, 3), plVec3I32(5, 6, 7));

    TestSerialization<plTestStruct>(data);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTestClass2")
  {
    plTestClass2 Instance;
    const plRTTI* pRtti = plGetStaticRTTI<plTestClass2>();

    {
      TestMemberProperty<const char*>("Text", &Instance, pRtti, plPropertyFlags::StandardType | plPropertyFlags::Const, plString("Legen"), plString("dary"));
      const plAbstractProperty* pProp = pRtti->FindPropertyByName("SubVector", false);
      PLASMA_TEST_BOOL(pProp == nullptr);
    }

    {
      TestMemberProperty<plVec3>("SubVector", &Instance, pRtti, plPropertyFlags::StandardType | plPropertyFlags::ReadOnly, plVec3(3, 4, 5), plVec3(3, 4, 5));
      const plAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct", false);
      PLASMA_TEST_BOOL(pProp == nullptr);
    }

    {
      const plAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct");
      PLASMA_TEST_BOOL(pProp != nullptr);

      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
      plAbstractMemberProperty* pAbs = (plAbstractMemberProperty*)pProp;

      const plRTTI* pStruct = pAbs->GetSpecificType();
      void* pSubStruct = pAbs->GetPropertyPointer(&Instance);

      PLASMA_TEST_BOOL(pSubStruct != nullptr);

      TestMemberProperty<float>("Float", pSubStruct, pStruct, plPropertyFlags::StandardType, 33.3f, 44.4f, false);
    }

    TestSerialization<plTestClass2>(Instance);
  }
}


PLASMA_CREATE_SIMPLE_TEST(Reflection, Enum)
{
  const plRTTI* pEnumRTTI = plGetStaticRTTI<plExampleEnum>();
  const plRTTI* pRTTI = plGetStaticRTTI<plTestEnumStruct>();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enum Constants")
  {
    PLASMA_TEST_BOOL(pEnumRTTI->IsDerivedFrom<plEnumBase>());
    auto props = pEnumRTTI->GetProperties();
    PLASMA_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Constant);
      auto pConstantProp = static_cast<const plAbstractConstantProperty*>(pProp);
      PLASMA_TEST_BOOL(pConstantProp->GetSpecificType() == plGetStaticRTTI<plInt8>());
    }
    PLASMA_TEST_INT(plExampleEnum::Default, plReflectionUtils::DefaultEnumerationValue(pEnumRTTI));

    PLASMA_TEST_STRING(props[0]->GetPropertyName(), "plExampleEnum::Default");
    PLASMA_TEST_STRING(props[1]->GetPropertyName(), "plExampleEnum::Value1");
    PLASMA_TEST_STRING(props[2]->GetPropertyName(), "plExampleEnum::Value2");
    PLASMA_TEST_STRING(props[3]->GetPropertyName(), "plExampleEnum::Value3");

    auto pTypedConstantProp0 = static_cast<const plTypedConstantProperty<plInt8>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const plTypedConstantProperty<plInt8>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const plTypedConstantProperty<plInt8>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const plTypedConstantProperty<plInt8>*>(props[3]);
    PLASMA_TEST_INT(pTypedConstantProp0->GetValue(), plExampleEnum::Default);
    PLASMA_TEST_INT(pTypedConstantProp1->GetValue(), plExampleEnum::Value1);
    PLASMA_TEST_INT(pTypedConstantProp2->GetValue(), plExampleEnum::Value2);
    PLASMA_TEST_INT(pTypedConstantProp3->GetValue(), plExampleEnum::Value3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enum Property")
  {
    plTestEnumStruct data;
    auto props = pRTTI->GetProperties();
    PLASMA_TEST_INT(props.GetCount(), 4);

    for (auto pProp : props)
    {
      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
      auto pMemberProp = static_cast<const plAbstractMemberProperty*>(pProp);
      PLASMA_TEST_INT(pMemberProp->GetFlags().GetValue(), plPropertyFlags::IsEnum);
      PLASMA_TEST_BOOL(pMemberProp->GetSpecificType() == pEnumRTTI);
      auto pEnumProp = static_cast<const plAbstractEnumerationProperty*>(pProp);
      PLASMA_TEST_BOOL(pEnumProp->GetValue(&data) == plExampleEnum::Value1);

      const plRTTI* pEnumPropertyRTTI = pEnumProp->GetSpecificType();
      // Set and get all valid enum values.
      for (auto pProp2 : pEnumPropertyRTTI->GetProperties().GetSubArray(1))
      {
        auto pConstantProp = static_cast<const plTypedConstantProperty<plInt8>*>(pProp2);
        pEnumProp->SetValue(&data, pConstantProp->GetValue());
        PLASMA_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        // Enum <-> string
        plStringBuilder sValue;
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue));
        PLASMA_TEST_STRING(sValue, pConstantProp->GetPropertyName());

        // Setting the value via a string also works.
        pEnumProp->SetValue(&data, plExampleEnum::Value1);
        plReflectionUtils::SetMemberPropertyValue(pEnumProp, &data, sValue.GetData());
        PLASMA_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        plInt64 iValue = 0;
        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(
          pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, plReflectionUtils::EnumConversionMode::ValueNameOnly));
        PLASMA_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) ||
                     sValue.IsEqual(plStringUtils::FindLastSubString(pConstantProp->GetPropertyName(), "::") + 2));

        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(
          pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, plReflectionUtils::EnumConversionMode::ValueNameOnly));
        PLASMA_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) ||
                     sValue.IsEqual(plStringUtils::FindLastSubString(pConstantProp->GetPropertyName(), "::") + 2));

        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, pConstantProp->GetValue());

        PLASMA_TEST_INT(iValue, plReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue));
        PLASMA_TEST_INT(plExampleEnum::Default, plReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue + 666));
      }
    }

    PLASMA_TEST_BOOL(data.m_enum == plExampleEnum::Value3);
    PLASMA_TEST_BOOL(data.m_enumClass == plExampleEnum::Value3);

    PLASMA_TEST_BOOL(data.GetEnum() == plExampleEnum::Value3);
    PLASMA_TEST_BOOL(data.GetEnumClass() == plExampleEnum::Value3);

    TestSerialization<plTestEnumStruct>(data);
  }
}


PLASMA_CREATE_SIMPLE_TEST(Reflection, Bitflags)
{
  const plRTTI* pBitflagsRTTI = plGetStaticRTTI<plExampleBitflags>();
  const plRTTI* pRTTI = plGetStaticRTTI<plTestBitflagsStruct>();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Bitflags Constants")
  {
    PLASMA_TEST_BOOL(pBitflagsRTTI->IsDerivedFrom<plBitflagsBase>());
    auto props = pBitflagsRTTI->GetProperties();
    PLASMA_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Constant);
      PLASMA_TEST_BOOL(pProp->GetSpecificType() == plGetStaticRTTI<plUInt64>());
    }
    PLASMA_TEST_INT(plExampleBitflags::Default, plReflectionUtils::DefaultEnumerationValue(pBitflagsRTTI));

    PLASMA_TEST_STRING(props[0]->GetPropertyName(), "plExampleBitflags::Default");
    PLASMA_TEST_STRING(props[1]->GetPropertyName(), "plExampleBitflags::Value1");
    PLASMA_TEST_STRING(props[2]->GetPropertyName(), "plExampleBitflags::Value2");
    PLASMA_TEST_STRING(props[3]->GetPropertyName(), "plExampleBitflags::Value3");

    auto pTypedConstantProp0 = static_cast<const plTypedConstantProperty<plUInt64>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const plTypedConstantProperty<plUInt64>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const plTypedConstantProperty<plUInt64>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const plTypedConstantProperty<plUInt64>*>(props[3]);
    PLASMA_TEST_BOOL(pTypedConstantProp0->GetValue() == plExampleBitflags::Default);
    PLASMA_TEST_BOOL(pTypedConstantProp1->GetValue() == plExampleBitflags::Value1);
    PLASMA_TEST_BOOL(pTypedConstantProp2->GetValue() == plExampleBitflags::Value2);
    PLASMA_TEST_BOOL(pTypedConstantProp3->GetValue() == plExampleBitflags::Value3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Bitflags Property")
  {
    plTestBitflagsStruct data;
    auto props = pRTTI->GetProperties();
    PLASMA_TEST_INT(props.GetCount(), 2);

    for (auto pProp : props)
    {
      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
      PLASMA_TEST_BOOL(pProp->GetSpecificType() == pBitflagsRTTI);
      PLASMA_TEST_INT(pProp->GetFlags().GetValue(), plPropertyFlags::Bitflags);
      auto pBitflagsProp = static_cast<const plAbstractEnumerationProperty*>(pProp);
      PLASMA_TEST_BOOL(pBitflagsProp->GetValue(&data) == plExampleBitflags::Value1);

      const plRTTI* pBitflagsPropertyRTTI = pBitflagsProp->GetSpecificType();

      // Set and get all valid bitflags values. (skip default value)
      plUInt64 constants[] = {static_cast<const plTypedConstantProperty<plUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[1])->GetValue(),
        static_cast<const plTypedConstantProperty<plUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[2])->GetValue(),
        static_cast<const plTypedConstantProperty<plUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[3])->GetValue()};

      const char* stringValues[] = {"",
        "plExampleBitflags::Value1",
        "plExampleBitflags::Value2",
        "plExampleBitflags::Value1|plExampleBitflags::Value2",
        "plExampleBitflags::Value3",
        "plExampleBitflags::Value1|plExampleBitflags::Value3",
        "plExampleBitflags::Value2|plExampleBitflags::Value3",
        "plExampleBitflags::Value1|plExampleBitflags::Value2|plExampleBitflags::Value3"};

      const char* stringValuesShort[] = {"",
        "Value1",
        "Value2",
        "Value1|Value2",
        "Value3",
        "Value1|Value3",
        "Value2|Value3",
        "Value1|Value2|Value3"};
      for (plInt32 i = 0; i < 8; ++i)
      {
        plUInt64 uiBitflagValue = 0;
        uiBitflagValue |= (i & PLASMA_BIT(0)) != 0 ? constants[0] : 0;
        uiBitflagValue |= (i & PLASMA_BIT(1)) != 0 ? constants[1] : 0;
        uiBitflagValue |= (i & PLASMA_BIT(2)) != 0 ? constants[2] : 0;

        pBitflagsProp->SetValue(&data, uiBitflagValue);
        PLASMA_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        // Bitflags <-> string
        plStringBuilder sValue;
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue));
        PLASMA_TEST_STRING(sValue, stringValues[i]);

        // Setting the value via a string also works.
        pBitflagsProp->SetValue(&data, 0);
        plReflectionUtils::SetMemberPropertyValue(pBitflagsProp, &data, sValue.GetData());
        PLASMA_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        plInt64 iValue = 0;
        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(
          pBitflagsPropertyRTTI, uiBitflagValue, sValue, plReflectionUtils::EnumConversionMode::ValueNameOnly));
        PLASMA_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        PLASMA_TEST_BOOL(plReflectionUtils::EnumerationToString(
          pBitflagsPropertyRTTI, uiBitflagValue, sValue, plReflectionUtils::EnumConversionMode::ValueNameOnly));
        PLASMA_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        PLASMA_TEST_BOOL(plReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        PLASMA_TEST_INT(iValue, uiBitflagValue);

        PLASMA_TEST_INT(iValue, plReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue));
        PLASMA_TEST_INT(iValue, plReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue | PLASMA_BIT(16)));
      }
    }

    PLASMA_TEST_BOOL(data.m_bitflagsClass == (plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3));
    PLASMA_TEST_BOOL(data.GetBitflagsClass() == (plExampleBitflags::Value1 | plExampleBitflags::Value2 | plExampleBitflags::Value3));
    TestSerialization<plTestBitflagsStruct>(data);
  }
}


template <typename T>
void TestArrayPropertyVariant(const plAbstractArrayProperty* pArrayProp, void* pObject, const plRTTI* pRtti, T& value)
{
  T temp = {};

  // Reflection Utils
  plVariant value0 = plReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 0);
  PLASMA_TEST_BOOL(value0 == plVariant(value));
  // insert
  plReflectionUtils::InsertArrayPropertyValue(pArrayProp, pObject, plVariant(temp), 2);
  PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 3);
  plVariant value2 = plReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  PLASMA_TEST_BOOL(value0 != value2);
  plReflectionUtils::SetArrayPropertyValue(pArrayProp, pObject, 2, value);
  value2 = plReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  PLASMA_TEST_BOOL(value0 == value2);
  // remove again
  plReflectionUtils::RemoveArrayPropertyValue(pArrayProp, pObject, 2);
  PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 2);
}

template <>
void TestArrayPropertyVariant<plTestArrays>(const plAbstractArrayProperty* pArrayProp, void* pObject, const plRTTI* pRtti, plTestArrays& value)
{
}

template <>
void TestArrayPropertyVariant<plTestStruct3>(const plAbstractArrayProperty* pArrayProp, void* pObject, const plRTTI* pRtti, plTestStruct3& value)
{
}

template <typename T>
void TestArrayProperty(const char* szPropName, void* pObject, const plRTTI* pRtti, T& value)
{
  const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  PLASMA_TEST_BOOL(pProp != nullptr);
  PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Array);
  auto pArrayProp = static_cast<const plAbstractArrayProperty*>(pProp);
  const plRTTI* pElemRtti = pProp->GetSpecificType();
  PLASMA_TEST_BOOL(pElemRtti == plGetStaticRTTI<T>());
  if (!pArrayProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
  {
    // If we don't know the element type T but we can allocate it, we can handle it anyway.
    if (pElemRtti->GetAllocator()->CanAllocate())
    {
      void* pData = pElemRtti->GetAllocator()->Allocate<void>();

      pArrayProp->SetCount(pObject, 2);
      PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 2);
      // Push default constructed object in both slots.
      pArrayProp->SetValue(pObject, 0, pData);
      pArrayProp->SetValue(pObject, 1, pData);

      // Retrieve it again and compare to function parameter, they should be different.
      pArrayProp->GetValue(pObject, 0, pData);
      PLASMA_TEST_BOOL(*static_cast<T*>(pData) != value);
      pArrayProp->GetValue(pObject, 1, pData);
      PLASMA_TEST_BOOL(*static_cast<T*>(pData) != value);

      pElemRtti->GetAllocator()->Deallocate(pData);
    }

    pArrayProp->Clear(pObject);
    PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 0);
    pArrayProp->SetCount(pObject, 2);
    pArrayProp->SetValue(pObject, 0, &value);
    pArrayProp->SetValue(pObject, 1, &value);

    // Insert default init values
    T temp = {};
    pArrayProp->Insert(pObject, 2, &temp);
    PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Insert(pObject, 0, &temp);
    PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 4);

    // Remove them again
    pArrayProp->Remove(pObject, 3);
    PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Remove(pObject, 0);
    PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 2);

    TestArrayPropertyVariant<T>(pArrayProp, pObject, pRtti, value);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  PLASMA_TEST_INT(pArrayProp->GetCount(pObject), 2);

  T v1 = {};
  pArrayProp->GetValue(pObject, 0, &v1);
  if constexpr (std::is_same<const char*, T>::value)
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(v1, value));
  }
  else
  {
    PLASMA_TEST_BOOL(v1 == value);
  }

  T v2 = {};
  pArrayProp->GetValue(pObject, 1, &v2);
  if constexpr (std::is_same<const char*, T>::value)
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(v2, value));
  }
  else
  {
    PLASMA_TEST_BOOL(v2 == value);
  }

  if (pElemRtti->GetAllocator()->CanAllocate())
  {
    // Current values should be different from default constructed version.
    void* pData = pElemRtti->GetAllocator()->Allocate<void>();

    PLASMA_TEST_BOOL(*static_cast<T*>(pData) != v1);
    PLASMA_TEST_BOOL(*static_cast<T*>(pData) != v2);

    pElemRtti->GetAllocator()->Deallocate(pData);
  }
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, Arrays)
{
  plTestArrays containers;
  const plRTTI* pRtti = plGetStaticRTTI<plTestArrays>();
  PLASMA_TEST_BOOL(pRtti != nullptr);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "POD Array")
  {
    double fValue = 5;
    TestArrayProperty<double>("Hybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("HybridRO", &containers, pRtti, fValue);

    TestArrayProperty<double>("AcHybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("AcHybridRO", &containers, pRtti, fValue);

    const char* szValue = "Bla";
    const char* szValue2 = "LongString------------------------------------------------------------------------------------";
    plString sValue = szValue;
    plString sValue2 = szValue2;

    TestArrayProperty<plString>("HybridChar", &containers, pRtti, sValue);
    TestArrayProperty<plString>("HybridCharRO", &containers, pRtti, sValue);

    TestArrayProperty<const char*>("AcHybridChar", &containers, pRtti, szValue);
    TestArrayProperty<const char*>("AcHybridCharRO", &containers, pRtti, szValue);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Struct Array")
  {
    plTestStruct3 data;
    data.m_fFloat1 = 99.0f;
    data.m_UInt8 = 127;

    TestArrayProperty<plTestStruct3>("Dynamic", &containers, pRtti, data);
    TestArrayProperty<plTestStruct3>("DynamicRO", &containers, pRtti, data);

    TestArrayProperty<plTestStruct3>("AcDynamic", &containers, pRtti, data);
    TestArrayProperty<plTestStruct3>("AcDynamicRO", &containers, pRtti, data);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plReflectedClass Array")
  {
    plTestArrays data;
    data.m_Hybrid.PushBack(42.0);

    TestArrayProperty<plTestArrays>("Deque", &containers, pRtti, data);
    TestArrayProperty<plTestArrays>("DequeRO", &containers, pRtti, data);

    TestArrayProperty<plTestArrays>("AcDeque", &containers, pRtti, data);
    TestArrayProperty<plTestArrays>("AcDequeRO", &containers, pRtti, data);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Custom Variant Array")
  {
    plVarianceTypeAngle data{0.1f, plAngle::Degree(45.0f)};

    TestArrayProperty<plVarianceTypeAngle>("Custom", &containers, pRtti, data);
    TestArrayProperty<plVarianceTypeAngle>("CustomRO", &containers, pRtti, data);

    TestArrayProperty<plVarianceTypeAngle>("AcCustom", &containers, pRtti, data);
    TestArrayProperty<plVarianceTypeAngle>("AcCustomRO", &containers, pRtti, data);
  }

  TestSerialization<plTestArrays>(containers);
}



template <typename T>
void TestSetProperty(const char* szPropName, void* pObject, const plRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!PLASMA_TEST_BOOL(pProp != nullptr))
    return;

  PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Set);
  auto pSetProp = static_cast<const plAbstractSetProperty*>(pProp);
  const plRTTI* pElemRtti = pProp->GetSpecificType();
  PLASMA_TEST_BOOL(pElemRtti == plGetStaticRTTI<T>());

  if (!pSetProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
  {
    pSetProp->Clear(pObject);
    PLASMA_TEST_BOOL(pSetProp->IsEmpty(pObject));
    pSetProp->Insert(pObject, &ref_value1);
    PLASMA_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    PLASMA_TEST_BOOL(!pSetProp->Contains(pObject, &ref_value2));
    pSetProp->Insert(pObject, &ref_value2);
    PLASMA_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));

    // Insert default init value
    if (!plIsPointer<T>::value)
    {
      T temp = T{};
      pSetProp->Insert(pObject, &temp);
      PLASMA_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
      PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));
      PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &temp));

      // Remove it again
      pSetProp->Remove(pObject, &temp);
      PLASMA_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      PLASMA_TEST_BOOL(!pSetProp->Contains(pObject, &temp));
    }
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  PLASMA_TEST_BOOL(!pSetProp->IsEmpty(pObject));
  PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
  PLASMA_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));


  plHybridArray<plVariant, 16> keys;
  pSetProp->GetValues(pObject, keys);
  PLASMA_TEST_INT(keys.GetCount(), 2);
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, Sets)
{
  plTestSets containers;
  const plRTTI* pRtti = plGetStaticRTTI<plTestSets>();
  PLASMA_TEST_BOOL(pRtti != nullptr);

  // Disabled because MSVC 2017 has code generation issues in Release builds
  PLASMA_TEST_BLOCK(plTestBlock::Disabled, "plSet")
  {
    plInt8 iValue1 = -5;
    plInt8 iValue2 = 127;
    TestSetProperty<plInt8>("Set", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<plInt8>("SetRO", &containers, pRtti, iValue1, iValue2);

    double fValue1 = 5;
    double fValue2 = -3;
    TestSetProperty<double>("AcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<double>("AcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plHashSet")
  {
    plInt32 iValue1 = -5;
    plInt32 iValue2 = 127;
    TestSetProperty<plInt32>("HashSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<plInt32>("HashSetRO", &containers, pRtti, iValue1, iValue2);

    plInt64 fValue1 = 5;
    plInt64 fValue2 = -3;
    TestSetProperty<plInt64>("HashAcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<plInt64>("HashAcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plDeque Pseudo Set")
  {
    int iValue1 = -5;
    int iValue2 = 127;

    TestSetProperty<int>("AcPseudoSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<int>("AcPseudoSetRO", &containers, pRtti, iValue1, iValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plSetPtr Pseudo Set")
  {
    plString sValue1 = "TestString1";
    plString sValue2 = "Test String Deus";

    TestSetProperty<plString>("AcPseudoSet2", &containers, pRtti, sValue1, sValue2);
    TestSetProperty<plString>("AcPseudoSet2RO", &containers, pRtti, sValue1, sValue2);

    const char* szValue1 = "TestString1";
    const char* szValue2 = "Test String Deus";
    TestSetProperty<const char*>("AcPseudoSet2b", &containers, pRtti, szValue1, szValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Custom Variant HashSet")
  {
    plVarianceTypeAngle value1{-0.1f, plAngle::Degree(-45.0f)};
    plVarianceTypeAngle value2{0.1f, plAngle::Degree(45.0f)};

    TestSetProperty<plVarianceTypeAngle>("CustomHashSet", &containers, pRtti, value1, value2);
    TestSetProperty<plVarianceTypeAngle>("CustomHashSetRO", &containers, pRtti, value1, value2);

    plVarianceTypeAngle value3{-0.2f, plAngle::Degree(-90.0f)};
    plVarianceTypeAngle value4{0.2f, plAngle::Degree(90.0f)};
    TestSetProperty<plVarianceTypeAngle>("CustomHashAcSet", &containers, pRtti, value3, value4);
    TestSetProperty<plVarianceTypeAngle>("CustomHashAcSetRO", &containers, pRtti, value3, value4);
  }
  TestSerialization<plTestSets>(containers);
}

template <typename T>
void TestMapProperty(const char* szPropName, void* pObject, const plRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  PLASMA_TEST_BOOL(pProp != nullptr);
  PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Map);
  auto pMapProp = static_cast<const plAbstractMapProperty*>(pProp);
  const plRTTI* pElemRtti = pProp->GetSpecificType();
  PLASMA_TEST_BOOL(pElemRtti == plGetStaticRTTI<T>());
  PLASMA_TEST_BOOL(plReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == plGetStaticRTTI<plVariant>() || pElemRtti == plGetStaticRTTI<plVarianceTypeAngle>());

  if (!pMapProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
  {
    pMapProp->Clear(pObject);
    PLASMA_TEST_BOOL(pMapProp->IsEmpty(pObject));
    pMapProp->Insert(pObject, "value1", &ref_value1);
    PLASMA_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    PLASMA_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    PLASMA_TEST_BOOL(!pMapProp->Contains(pObject, "value2"));
    T getValue;
    PLASMA_TEST_BOOL(!pMapProp->GetValue(pObject, "value2", &getValue));
    PLASMA_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    PLASMA_TEST_BOOL(getValue == ref_value1);

    pMapProp->Insert(pObject, "value2", &ref_value2);
    PLASMA_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    PLASMA_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    PLASMA_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
    PLASMA_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    PLASMA_TEST_BOOL(getValue == ref_value1);
    PLASMA_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue));
    PLASMA_TEST_BOOL(getValue == ref_value2);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  T getValue2;
  PLASMA_TEST_BOOL(!pMapProp->IsEmpty(pObject));
  PLASMA_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
  PLASMA_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
  PLASMA_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue2));
  PLASMA_TEST_BOOL(getValue2 == ref_value1);
  PLASMA_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue2));
  PLASMA_TEST_BOOL(getValue2 == ref_value2);

  plHybridArray<plString, 16> keys;
  pMapProp->GetKeys(pObject, keys);
  PLASMA_TEST_INT(keys.GetCount(), 2);
  keys.Sort();
  PLASMA_TEST_BOOL(keys[0] == "value1");
  PLASMA_TEST_BOOL(keys[1] == "value2");
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, Maps)
{
  plTestMaps containers;
  const plRTTI* pRtti = plGetStaticRTTI<plTestMaps>();
  PLASMA_TEST_BOOL(pRtti != nullptr);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMap")
  {
    int iValue1 = -5;
    int iValue2 = 127;
    TestMapProperty<int>("Map", &containers, pRtti, iValue1, iValue2);
    TestMapProperty<int>("MapRO", &containers, pRtti, iValue1, iValue2);

    plInt64 iValue1b = 5;
    plInt64 iValue2b = -3;
    TestMapProperty<plInt64>("AcMap", &containers, pRtti, iValue1b, iValue2b);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plHashMap")
  {
    double fValue1 = -5;
    double fValue2 = 127;
    TestMapProperty<double>("HashTable", &containers, pRtti, fValue1, fValue2);
    TestMapProperty<double>("HashTableRO", &containers, pRtti, fValue1, fValue2);

    plString sValue1 = "Bla";
    plString sValue2 = "Test";
    TestMapProperty<plString>("AcHashTable", &containers, pRtti, sValue1, sValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Accessor")
  {
    plVariant sValue1 = "Test";
    plVariant sValue2 = plVec4(1, 2, 3, 4);
    TestMapProperty<plVariant>("Accessor", &containers, pRtti, sValue1, sValue2);
    TestMapProperty<plVariant>("AccessorRO", &containers, pRtti, sValue1, sValue2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CustomVariant")
  {
    plVarianceTypeAngle value1{-0.1f, plAngle::Degree(-45.0f)};
    plVarianceTypeAngle value2{0.1f, plAngle::Degree(45.0f)};

    TestMapProperty<plVarianceTypeAngle>("CustomVariant", &containers, pRtti, value1, value2);
    TestMapProperty<plVarianceTypeAngle>("CustomVariantRO", &containers, pRtti, value1, value2);
  }
  TestSerialization<plTestMaps>(containers);
}


template <typename T>
void TestPointerMemberProperty(const char* szPropName, void* pObject, const plRTTI* pRtti, plBitflags<plPropertyFlags> expectedFlags, T* pExpectedValue)
{
  const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  PLASMA_TEST_BOOL(pProp != nullptr);
  PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
  auto pAbsMember = (const plAbstractMemberProperty*)pProp;
  PLASMA_TEST_INT(pProp->GetFlags().GetValue(), expectedFlags.GetValue());
  PLASMA_TEST_BOOL(pProp->GetSpecificType() == plGetStaticRTTI<T>());
  void* pData = nullptr;
  pAbsMember->GetValuePtr(pObject, &pData);
  PLASMA_TEST_BOOL(pData == pExpectedValue);

  // Set value to null.
  {
    void* pDataNull = nullptr;
    pAbsMember->SetValuePtr(pObject, &pDataNull);
    void* pDataNull2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pDataNull2);
    PLASMA_TEST_BOOL(pDataNull == pDataNull2);
  }

  // Set value to new instance.
  {
    void* pNewData = pAbsMember->GetSpecificType()->GetAllocator()->Allocate<void>();
    pAbsMember->SetValuePtr(pObject, &pNewData);
    void* pData2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pData2);
    PLASMA_TEST_BOOL(pNewData == pData2);
  }

  // Delete old value
  pAbsMember->GetSpecificType()->GetAllocator()->Deallocate(pData);
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, Pointer)
{
  const plRTTI* pRtti = plGetStaticRTTI<plTestPtr>();
  PLASMA_TEST_BOOL(pRtti != nullptr);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Member Property Ptr")
  {
    plTestPtr containers;
    {
      const plAbstractProperty* pProp = pRtti->FindPropertyByName("ConstCharPtr");
      PLASMA_TEST_BOOL(pProp != nullptr);
      PLASMA_TEST_BOOL(pProp->GetCategory() == plPropertyCategory::Member);
      PLASMA_TEST_INT(pProp->GetFlags().GetValue(), (plPropertyFlags::StandardType | plPropertyFlags::Const).GetValue());
      PLASMA_TEST_BOOL(pProp->GetSpecificType() == plGetStaticRTTI<const char*>());
    }

    TestPointerMemberProperty<plTestArrays>(
      "ArraysPtr", &containers, pRtti, plPropertyFlags::Class | plPropertyFlags::Pointer | plPropertyFlags::PointerOwner, containers.m_pArrays);
    TestPointerMemberProperty<plTestArrays>("ArraysPtrDirect", &containers, pRtti,
      plPropertyFlags::Class | plPropertyFlags::Pointer | plPropertyFlags::PointerOwner, containers.m_pArraysDirect);
  }

  plTestPtr containers;
  plDefaultMemoryStreamStorage StreamStorage;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Serialize Property Ptr")
  {
    containers.m_sString = "Test";

    containers.m_pArrays = PLASMA_DEFAULT_NEW(plTestArrays);
    containers.m_pArrays->m_Deque.PushBack(plTestArrays());

    containers.m_ArrayPtr.PushBack(PLASMA_DEFAULT_NEW(plTestArrays));
    containers.m_ArrayPtr[0]->m_Hybrid.PushBack(5.0);

    containers.m_SetPtr.Insert(PLASMA_DEFAULT_NEW(plTestSets));
    containers.m_SetPtr.GetIterator().Key()->m_Array.PushBack("BLA");
  }

  TestSerialization<plTestPtr>(containers);
}
