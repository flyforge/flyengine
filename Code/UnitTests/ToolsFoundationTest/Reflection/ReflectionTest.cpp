#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Reflection);


void VariantToPropertyTest(void* pIntStruct, const plRTTI* pRttiInt, const char* szPropName, plVariant::Type::Enum type)
{
  const plAbstractMemberProperty* pProp = plReflectionUtils::GetMemberProperty(pRttiInt, szPropName);
  PLASMA_TEST_BOOL(pProp != nullptr);
  if (pProp)
  {
    plVariant oldValue = plReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    PLASMA_TEST_BOOL(oldValue.IsValid());
    PLASMA_TEST_BOOL(oldValue.GetType() == type);

    plVariant defaultValue = plReflectionUtils::GetDefaultValue(pProp);
    PLASMA_TEST_BOOL(defaultValue.GetType() == type);
    plReflectionUtils::SetMemberPropertyValue(pProp, pIntStruct, defaultValue);

    plVariant newValue = plReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    PLASMA_TEST_BOOL(newValue.IsValid());
    PLASMA_TEST_BOOL(newValue.GetType() == type);
    PLASMA_TEST_BOOL(newValue == defaultValue);
    PLASMA_TEST_BOOL(newValue != oldValue);
  }
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, ReflectionUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Integer Properties")
  {
    plIntegerStruct intStruct;
    const plRTTI* pRttiInt = plRTTI::FindTypeByName("plIntegerStruct");
    PLASMA_TEST_BOOL(pRttiInt != nullptr);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int8", plVariant::Type::Int8);
    PLASMA_TEST_INT(0, intStruct.GetInt8());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt8", plVariant::Type::UInt8);
    PLASMA_TEST_INT(0, intStruct.GetUInt8());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int16", plVariant::Type::Int16);
    PLASMA_TEST_INT(0, intStruct.m_iInt16);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt16", plVariant::Type::UInt16);
    PLASMA_TEST_INT(0, intStruct.m_iUInt16);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int32", plVariant::Type::Int32);
    PLASMA_TEST_INT(0, intStruct.GetInt32());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt32", plVariant::Type::UInt32);
    PLASMA_TEST_INT(0, intStruct.GetUInt32());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int64", plVariant::Type::Int64);
    PLASMA_TEST_INT(0, intStruct.m_iInt64);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt64", plVariant::Type::UInt64);
    PLASMA_TEST_INT(0, intStruct.m_iUInt64);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Float Properties")
  {
    plFloatStruct floatStruct;
    const plRTTI* pRttiFloat = plRTTI::FindTypeByName("plFloatStruct");
    PLASMA_TEST_BOOL(pRttiFloat != nullptr);

    VariantToPropertyTest(&floatStruct, pRttiFloat, "Float", plVariant::Type::Float);
    PLASMA_TEST_FLOAT(0, floatStruct.GetFloat(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Double", plVariant::Type::Double);
    PLASMA_TEST_FLOAT(0, floatStruct.GetDouble(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Time", plVariant::Type::Time);
    PLASMA_TEST_FLOAT(0, floatStruct.GetTime().GetSeconds(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Angle", plVariant::Type::Angle);
    PLASMA_TEST_FLOAT(0, floatStruct.GetAngle().GetDegree(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Misc Properties")
  {
    plPODClass podClass;
    const plRTTI* pRttiPOD = plRTTI::FindTypeByName("plPODClass");
    PLASMA_TEST_BOOL(pRttiPOD != nullptr);

    VariantToPropertyTest(&podClass, pRttiPOD, "Bool", plVariant::Type::Bool);
    PLASMA_TEST_BOOL(podClass.GetBool() == false);
    VariantToPropertyTest(&podClass, pRttiPOD, "Color", plVariant::Type::Color);
    PLASMA_TEST_BOOL(podClass.GetColor() == plColor(1.0f, 1.0f, 1.0f, 1.0f));
    VariantToPropertyTest(&podClass, pRttiPOD, "String", plVariant::Type::String);
    PLASMA_TEST_STRING(podClass.GetString(), "");
    VariantToPropertyTest(&podClass, pRttiPOD, "Buffer", plVariant::Type::DataBuffer);
    PLASMA_TEST_BOOL(podClass.GetBuffer() == plDataBuffer());
    VariantToPropertyTest(&podClass, pRttiPOD, "VarianceAngle", plVariant::Type::TypedObject);
    PLASMA_TEST_BOOL(podClass.GetCustom() == plVarianceTypeAngle{});
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Math Properties")
  {
    plMathClass mathClass;
    const plRTTI* pRttiMath = plRTTI::FindTypeByName("plMathClass");
    PLASMA_TEST_BOOL(pRttiMath != nullptr);

    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2", plVariant::Type::Vector2);
    PLASMA_TEST_BOOL(mathClass.GetVec2() == plVec2(0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3", plVariant::Type::Vector3);
    PLASMA_TEST_BOOL(mathClass.GetVec3() == plVec3(0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4", plVariant::Type::Vector4);
    PLASMA_TEST_BOOL(mathClass.GetVec4() == plVec4(0.0f, 0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2I", plVariant::Type::Vector2I);
    PLASMA_TEST_BOOL(mathClass.m_Vec2I == plVec2I32(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3I", plVariant::Type::Vector3I);
    PLASMA_TEST_BOOL(mathClass.m_Vec3I == plVec3I32(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4I", plVariant::Type::Vector4I);
    PLASMA_TEST_BOOL(mathClass.m_Vec4I == plVec4I32(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quat", plVariant::Type::Quaternion);
    PLASMA_TEST_BOOL(mathClass.GetQuat() == plQuat(0.0f, 0.0f, 0.0f, 1.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3", plVariant::Type::Matrix3);
    PLASMA_TEST_BOOL(mathClass.GetMat3() == plMat3::IdentityMatrix());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4", plVariant::Type::Matrix4);
    PLASMA_TEST_BOOL(mathClass.GetMat4() == plMat4::IdentityMatrix());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enumeration Properties")
  {
    plEnumerationsClass enumClass;
    const plRTTI* pRttiEnum = plRTTI::FindTypeByName("plEnumerationsClass");
    PLASMA_TEST_BOOL(pRttiEnum != nullptr);

    VariantToPropertyTest(&enumClass, pRttiEnum, "Enum", plVariant::Type::Int64);
    PLASMA_TEST_BOOL(enumClass.GetEnum() == plExampleEnum::Value1);
    VariantToPropertyTest(&enumClass, pRttiEnum, "Bitflags", plVariant::Type::Int64);
    PLASMA_TEST_BOOL(enumClass.GetBitflags() == plExampleBitflags::Value1);
  }
}

void AccessorPropertyTest(plIReflectedTypeAccessor& ref_accessor, const char* szProperty, plVariant::Type::Enum type)
{
  plVariant oldValue = ref_accessor.GetValue(szProperty);
  PLASMA_TEST_BOOL(oldValue.IsValid());
  PLASMA_TEST_BOOL(oldValue.GetType() == type);

  const plAbstractProperty* pProp = ref_accessor.GetType()->FindPropertyByName(szProperty);
  plVariant defaultValue = plReflectionUtils::GetDefaultValue(pProp);
  PLASMA_TEST_BOOL(defaultValue.GetType() == type);
  bool bSetSuccess = ref_accessor.SetValue(szProperty, defaultValue);
  PLASMA_TEST_BOOL(bSetSuccess);

  plVariant newValue = ref_accessor.GetValue(szProperty);
  PLASMA_TEST_BOOL(newValue.IsValid());
  PLASMA_TEST_BOOL(newValue.GetType() == type);
  PLASMA_TEST_BOOL(newValue == defaultValue);
}

plUInt32 AccessorPropertiesTest(plIReflectedTypeAccessor& ref_accessor, const plRTTI* pType)
{
  plUInt32 uiPropertiesSet = 0;
  PLASMA_TEST_BOOL(pType != nullptr);

  // Call for base class
  if (pType->GetParentType() != nullptr)
  {
    uiPropertiesSet += AccessorPropertiesTest(ref_accessor, pType->GetParentType());
  }

  // Test properties
  plUInt32 uiPropCount = pType->GetProperties().GetCount();
  for (plUInt32 i = 0; i < uiPropCount; ++i)
  {
    const plAbstractProperty* pProp = pType->GetProperties()[i];
    const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
      {
        auto pProp3 = static_cast<const plAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(plPropertyFlags::IsEnum))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), plVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Bitflags))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), plVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (bIsValueType)
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), pProp3->GetSpecificType()->GetVariantType());
          uiPropertiesSet++;
        }
        else // plPropertyFlags::Class
        {
          // Recurs into sub-classes
          const plUuid& subObjectGuid = ref_accessor.GetValue(pProp->GetPropertyName()).Get<plUuid>();
          plDocumentObject* pEmbeddedClassObject = const_cast<plDocumentObject*>(ref_accessor.GetOwner()->GetChild(subObjectGuid));
          uiPropertiesSet += AccessorPropertiesTest(pEmbeddedClassObject->GetTypeAccessor(), pProp3->GetSpecificType());
        }
      }
      break;
      case plPropertyCategory::Array:
      {
        // plAbstractArrayProperty* pProp3 = static_cast<plAbstractArrayProperty*>(pProp);
        // TODO
      }
      break;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
  return uiPropertiesSet;
}

plUInt32 AccessorPropertiesTest(plIReflectedTypeAccessor& ref_accessor)
{
  const plRTTI* handle = ref_accessor.GetType();
  return AccessorPropertiesTest(ref_accessor, handle);
}

static plUInt32 GetTypeCount()
{
  plUInt32 uiCount = 0;
  plRTTI::ForEachType([&](const plRTTI* pRtti) { uiCount++; });
  return uiCount;
}

static const plRTTI* RegisterType(const char* szTypeName)
{
  const plRTTI* pRtti = plRTTI::FindTypeByName(szTypeName);
  PLASMA_TEST_BOOL(pRtti != nullptr);

  plReflectedTypeDescriptor desc;
  plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  return plPhantomRttiManager::RegisterType(desc);
}

PLASMA_CREATE_SIMPLE_TEST(Reflection, ReflectedType)
{
  plTestDocumentObjectManager manager;

  /*const plRTTI* pRttiBase =*/RegisterType("plReflectedClass");
  /*const plRTTI* pRttiEnumBase =*/RegisterType("plEnumBase");
  /*const plRTTI* pRttiBitflagsBase =*/RegisterType("plBitflagsBase");

  const plRTTI* pRttiInt = RegisterType("plIntegerStruct");
  const plRTTI* pRttiFloat = RegisterType("plFloatStruct");
  const plRTTI* pRttiPOD = RegisterType("plPODClass");
  const plRTTI* pRttiMath = RegisterType("plMathClass");
  /*const plRTTI* pRttiEnum =*/RegisterType("plExampleEnum");
  /*const plRTTI* pRttiFlags =*/RegisterType("plExampleBitflags");
  const plRTTI* pRttiEnumerations = RegisterType("plEnumerationsClass");

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plReflectedTypeStorageAccessor")
  {
    {
      plDocumentObject* pObject = manager.CreateObject(pRttiInt);
      PLASMA_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 8);
      manager.DestroyObject(pObject);
    }
    {
      plDocumentObject* pObject = manager.CreateObject(pRttiFloat);
      PLASMA_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 4);
      manager.DestroyObject(pObject);
    }
    {
      plDocumentObject* pObject = manager.CreateObject(pRttiPOD);
      PLASMA_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 18);
      manager.DestroyObject(pObject);
    }
    {
      plDocumentObject* pObject = manager.CreateObject(pRttiMath);
      PLASMA_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 27);
      manager.DestroyObject(pObject);
    }
    {
      plDocumentObject* pObject = manager.CreateObject(pRttiEnumerations);
      PLASMA_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 2);
      manager.DestroyObject(pObject);
    }
  }
}


PLASMA_CREATE_SIMPLE_TEST(Reflection, ReflectedTypeReloading)
{
  plTestDocumentObjectManager manager;

  const plRTTI* pRttiInner = plRTTI::FindTypeByName("InnerStruct");
  const plRTTI* pRttiInnerP = nullptr;
  plReflectedTypeDescriptor descInner;

  const plRTTI* pRttiOuter = plRTTI::FindTypeByName("OuterClass");
  const plRTTI* pRttiOuterP = nullptr;
  plReflectedTypeDescriptor descOuter;

  plUInt32 uiRegisteredBaseTypes = GetTypeCount();
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RegisterType")
  {
    PLASMA_TEST_BOOL(pRttiInner != nullptr);
    plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    descInner.m_sTypeName = "InnerStructP";
    pRttiInnerP = plPhantomRttiManager::RegisterType(descInner);
    PLASMA_TEST_BOOL(pRttiInnerP != nullptr);

    PLASMA_TEST_BOOL(pRttiOuter != nullptr);
    plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    descOuter.m_sTypeName = "OuterClassP";
    descOuter.m_Properties[0].m_sType = "InnerStructP";
    pRttiOuterP = plPhantomRttiManager::RegisterType(descOuter);
    PLASMA_TEST_BOOL(pRttiOuterP != nullptr);
  }

  {
    plDocumentObject* pInnerObject = manager.CreateObject(pRttiInnerP);
    manager.AddObject(pInnerObject, nullptr, "Children", -1);
    plIReflectedTypeAccessor& innerAccessor = pInnerObject->GetTypeAccessor();

    plDocumentObject* pOuterObject = manager.CreateObject(pRttiOuterP);
    manager.AddObject(pOuterObject, nullptr, "Children", -1);
    plIReflectedTypeAccessor& outerAccessor = pOuterObject->GetTypeAccessor();

    plUuid innerGuid = outerAccessor.GetValue("Inner").Get<plUuid>();
    plDocumentObject* pEmbeddedInnerObject = manager.GetObject(innerGuid);
    plIReflectedTypeAccessor& embeddedInnerAccessor = pEmbeddedInnerObject->GetTypeAccessor();

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetValues")
    {
      // Just set a few values to make sure they don't get messed up by the following operations.
      PLASMA_TEST_BOOL(innerAccessor.SetValue("IP1", 1.4f));
      PLASMA_TEST_BOOL(outerAccessor.SetValue("OP1", 0.9f));
      PLASMA_TEST_BOOL(embeddedInnerAccessor.SetValue("IP1", 1.4f));
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AddProperty")
    {
      // Say we reload the engine and the InnerStruct now has a second property: IP2.
      descInner.m_Properties.PushBack(plReflectedPropertyDescriptor(plPropertyCategory::Member, "IP2", "plVec4",
        plBitflags<plPropertyFlags>(plPropertyFlags::StandardType), plArrayPtr<plPropertyAttribute* const>()));
      const plRTTI* NewInnerHandle = plPhantomRttiManager::RegisterType(descInner);
      PLASMA_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that the new property is present.
      AccessorPropertyTest(innerAccessor, "IP2", plVariant::Type::Vector4);

      AccessorPropertyTest(embeddedInnerAccessor, "IP2", plVariant::Type::Vector4);

      // Test that the old properties are still valid.
      PLASMA_TEST_BOOL(innerAccessor.GetValue("IP1") == 1.4f);
      PLASMA_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
      PLASMA_TEST_BOOL(embeddedInnerAccessor.GetValue("IP1") == 1.4f);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeProperty")
    {
      // Out original inner float now is a Int32!
      descInner.m_Properties[0].m_sType = "plInt32";
      const plRTTI* NewInnerHandle = plPhantomRttiManager::RegisterType(descInner);
      PLASMA_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Test if the previous value was converted correctly to its new type.
      plVariant innerValue = innerAccessor.GetValue("IP1");
      PLASMA_TEST_BOOL(innerValue.IsValid());
      PLASMA_TEST_BOOL(innerValue.GetType() == plVariant::Type::Int32);
      PLASMA_TEST_INT(innerValue.Get<plInt32>(), 1);

      plVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      PLASMA_TEST_BOOL(outerValue.IsValid());
      PLASMA_TEST_BOOL(outerValue.GetType() == plVariant::Type::Int32);
      PLASMA_TEST_INT(outerValue.Get<plInt32>(), 1);

      // Test that the old properties are still valid.
      PLASMA_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", plVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", plVariant::Type::Vector4);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DeleteProperty")
    {
      // Lets now delete the original inner property IP1.
      descInner.m_Properties.RemoveAtAndCopy(0);
      const plRTTI* NewInnerHandle = plPhantomRttiManager::RegisterType(descInner);
      PLASMA_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that IP1 is really gone.
      PLASMA_TEST_BOOL(!innerAccessor.GetValue("IP1").IsValid());
      PLASMA_TEST_BOOL(!embeddedInnerAccessor.GetValue("IP1").IsValid());

      // Test that the old properties are still valid.
      PLASMA_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", plVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", plVariant::Type::Vector4);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RevertProperties")
    {
      // Reset all classes to their initial state.
      plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
      descInner.m_sTypeName = "InnerStructP";
      plPhantomRttiManager::RegisterType(descInner);

      plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
      descInner.m_sTypeName = "OuterStructP";
      descOuter.m_Properties[0].m_sType = "InnerStructP";
      plPhantomRttiManager::RegisterType(descOuter);

      // Test that the old properties are back again.
      plStringBuilder path = "IP1";
      plVariant innerValue = innerAccessor.GetValue(path);
      PLASMA_TEST_BOOL(innerValue.IsValid());
      PLASMA_TEST_BOOL(innerValue.GetType() == plVariant::Type::Float);
      PLASMA_TEST_FLOAT(innerValue.Get<float>(), 1.0f, 0.0f);

      plVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      PLASMA_TEST_BOOL(outerValue.IsValid());
      PLASMA_TEST_BOOL(outerValue.GetType() == plVariant::Type::Float);
      PLASMA_TEST_FLOAT(outerValue.Get<float>(), 1.0f, 0.0f);
      PLASMA_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
    }

    manager.RemoveObject(pInnerObject);
    manager.DestroyObject(pInnerObject);

    manager.RemoveObject(pOuterObject);
    manager.DestroyObject(pOuterObject);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UnregisterType")
  {
    PLASMA_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes + 2);
    plPhantomRttiManager::UnregisterType(pRttiOuterP);
    plPhantomRttiManager::UnregisterType(pRttiInnerP);
    PLASMA_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes);
  }
}
