#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

void MirrorCheck(plTestDocument* pDoc, const plDocumentObject* pObject)
{
  // Create native object graph
  plAbstractObjectGraph graph;
  plAbstractObjectNode* pRootNode = nullptr;
  {
    plRttiConverterWriter rttiConverter(&graph, &pDoc->m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), pDoc->m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  plAbstractObjectGraph origGraph;
  plAbstractObjectNode* pOrigRootNode = nullptr;
  {
    plDocumentObjectConverterWriter writer(&origGraph, pDoc->GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  plDeque<plAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  PLASMA_TEST_BOOL(diffResult.GetCount() == 0);
}


plVariant GetVariantFromType(plVariant::Type::Enum type)
{
  switch (type)
  {
    case plVariant::Type::Invalid:
      return plVariant();
    case plVariant::Type::Bool:
      return plVariant(true);
    case plVariant::Type::Int8:
      return plVariant((plInt8)-55);
    case plVariant::Type::UInt8:
      return plVariant((plUInt8)44);
    case plVariant::Type::Int16:
      return plVariant((plInt16)-444);
    case plVariant::Type::UInt16:
      return plVariant((plUInt16)666);
    case plVariant::Type::Int32:
      return plVariant((plInt32)-88880);
    case plVariant::Type::UInt32:
      return plVariant((plUInt32)123445);
    case plVariant::Type::Int64:
      return plVariant((plInt64)-888800000);
    case plVariant::Type::UInt64:
      return plVariant((plUInt64)123445000);
    case plVariant::Type::Float:
      return plVariant(1024.0f);
    case plVariant::Type::Double:
      return plVariant(-2048.0f);
    case plVariant::Type::Color:
      return plVariant(plColor(0.5f, 33.0f, 2.0f, 0.3f));
    case plVariant::Type::ColorGamma:
      return plVariant(plColorGammaUB(plColor(0.5f, 33.0f, 2.0f, 0.3f)));
    case plVariant::Type::Vector2:
      return plVariant(plVec2(2.0f, 4.0f));
    case plVariant::Type::Vector3:
      return plVariant(plVec3(2.0f, 4.0f, -8.0f));
    case plVariant::Type::Vector4:
      return plVariant(plVec4(1.0f, 7.0f, 8.0f, -10.0f));
    case plVariant::Type::Vector2I:
      return plVariant(plVec2I32(1, 2));
    case plVariant::Type::Vector3I:
      return plVariant(plVec3I32(3, 4, 5));
    case plVariant::Type::Vector4I:
      return plVariant(plVec4I32(6, 7, 8, 9));
    case plVariant::Type::Quaternion:
    {
      plQuat quat;
      quat.SetFromEulerAngles(plAngle::Degree(30), plAngle::Degree(-15), plAngle::Degree(20));
      return plVariant(quat);
    }
    case plVariant::Type::Matrix3:
    {
      plMat3 mat = plMat3::IdentityMatrix();

      mat.SetRotationMatrix(plVec3(1.0f, 0.0f, 0.0f), plAngle::Degree(30));
      return plVariant(mat);
    }
    case plVariant::Type::Matrix4:
    {
      plMat4 mat = plMat4::IdentityMatrix();

      mat.SetRotationMatrix(plVec3(0.0f, 1.0f, 0.0f), plAngle::Degree(30));
      mat.SetTranslationVector(plVec3(1.0f, 2.0f, 3.0f));
      return plVariant(mat);
    }
    case plVariant::Type::String:
      return plVariant("Test");
    case plVariant::Type::StringView:
      return plVariant("Test");
    case plVariant::Type::Time:
      return plVariant(plTime::Seconds(123.0f));
    case plVariant::Type::Uuid:
    {
      plUuid guid;
      guid.CreateNewUuid();
      return plVariant(guid);
    }
    case plVariant::Type::Angle:
      return plVariant(plAngle::Degree(30.0f));
    case plVariant::Type::DataBuffer:
    {
      plDataBuffer data;
      data.PushBack(12);
      data.PushBack(55);
      data.PushBack(88);
      return plVariant(data);
    }
    case plVariant::Type::VariantArray:
      return plVariantArray();
    case plVariant::Type::VariantDictionary:
      return plVariantDictionary();
    case plVariant::Type::TypedPointer:
      return plVariant(plTypedPointer(nullptr, nullptr));
    case plVariant::Type::TypedObject:
      PLASMA_ASSERT_NOT_IMPLEMENTED;

    default:
      PLASMA_REPORT_FAILURE("Invalid case statement");
      return plVariant();
  }
  return plVariant();
}

void RecursiveModifyProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp, plObjectAccessorBase* pObjectAccessor)
{
  if (pProp->GetCategory() == plPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
    {
      if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
      {
        const plUuid oldGuid = pObjectAccessor->Get<plUuid>(pObject, pProp);
        plUuid newGuid;
        newGuid.CreateNewUuid();
        if (oldGuid.IsValid())
        {
          PLASMA_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).m_Result.Succeeded());
        }

        PLASMA_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, plVariant(), pProp->GetSpecificType(), newGuid).m_Result.Succeeded());

        const plDocumentObject* pChild = pObject->GetChild(newGuid);
        PLASMA_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
      }
      else
      {
        plVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        PLASMA_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).m_Result.Succeeded());
      }
    }
    else
    {
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags | plPropertyFlags::StandardType))
      {
        plVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        PLASMA_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).m_Result.Succeeded());
      }
      else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
      {
        // Noting to do here, value cannot change
      }
    }
  }
  else if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
  {
    if (pProp->GetFlags().IsAnySet(plPropertyFlags::StandardType | plPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
    {
      plInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      for (plInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, i).AssertSuccess();
      }

      plVariant value1 = plReflectionUtils::GetDefaultValue(pProp, 0);
      plVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      PLASMA_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, 0).m_Result.Succeeded());
      PLASMA_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, 1).m_Result.Succeeded());
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
    {
      plInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      plHybridArray<plVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (plInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        PLASMA_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<plUuid>())).m_Result.Succeeded());
      }

      if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        plUuid newGuid;
        newGuid.CreateNewUuid();
        PLASMA_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, 0, pProp->GetSpecificType(), newGuid).m_Result.Succeeded());
      }
    }
  }
  else if (pProp->GetCategory() == plPropertyCategory::Map)
  {
    if (pProp->GetFlags().IsAnySet(plPropertyFlags::StandardType | plPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
    {
      plInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      plHybridArray<plVariant, 16> keys;
      pObjectAccessor->GetKeys(pObject, pProp, keys).AssertSuccess();
      for (const plVariant& key : keys)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, key).AssertSuccess();
      }

      plVariant value1 = plReflectionUtils::GetDefaultValue(pProp, "Dummy");
      plVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      PLASMA_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, "value1").m_Result.Succeeded());
      PLASMA_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, "value2").m_Result.Succeeded());
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
    {
      plInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      plHybridArray<plVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (plInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        PLASMA_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<plUuid>())).m_Result.Succeeded());
      }

      plUuid newGuid;
      newGuid.CreateNewUuid();
      PLASMA_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, "value1", pProp->GetSpecificType(), newGuid).m_Result.Succeeded());
    }
  }
}

void RecursiveModifyObject(const plDocumentObject* pObject, plObjectAccessorBase* pAccessor)
{
  plHybridArray<const plAbstractProperty*, 32> Properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(Properties);
  for (const auto* pProp : Properties)
  {
    RecursiveModifyProperty(pObject, pProp, pAccessor);
  }

  for (const plDocumentObject* pSubObject : pObject->GetChildren())
  {
    RecursiveModifyObject(pSubObject, pAccessor);
  }
}

PLASMA_CREATE_SIMPLE_TEST(DocumentObject, ObjectMirror)
{
  plTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  plObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  plUuid mirrorGuid;

  pAccessor->StartTransaction("Init");
  plStatus status = pAccessor->AddObject(nullptr, (const plAbstractProperty*)nullptr, -1, plGetStaticRTTI<plMirrorTest>(), mirrorGuid);
  const plDocumentObject* pObject = pAccessor->GetObject(mirrorGuid);
  PLASMA_TEST_BOOL(status.m_Result.Succeeded());
  pAccessor->FinishTransaction();

  MirrorCheck(&doc, pObject);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Document Changes")
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
}
