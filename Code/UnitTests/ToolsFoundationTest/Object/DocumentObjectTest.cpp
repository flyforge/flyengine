#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(DocumentObject);

PLASMA_CREATE_SIMPLE_TEST(DocumentObject, DocumentObjectManager)
{
  plTestDocumentObjectManager manager;
  plDocumentObject* pObject = nullptr;
  plDocumentObject* pChildObject = nullptr;
  plDocumentObject* pChildren[4] = {nullptr};
  plDocumentObject* pSubElementObject[4] = {nullptr};

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DocumentObject")
  {
    PLASMA_TEST_BOOL(manager.CanAdd(plObjectTest::GetStaticRTTI(), nullptr, "", 0).m_Result.Succeeded());
    pObject = manager.CreateObject(plObjectTest::GetStaticRTTI());
    manager.AddObject(pObject, nullptr, "", 0);

    const char* szProperty = "SubObjectSet";
    PLASMA_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, 0).m_Result.Failed());
    PLASMA_TEST_BOOL(manager.CanAdd(plObjectTest::GetStaticRTTI(), pObject, szProperty, 0).m_Result.Succeeded());
    pChildObject = manager.CreateObject(plObjectTest::GetStaticRTTI());
    manager.AddObject(pChildObject, pObject, "SubObjectSet", 0);
    PLASMA_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 1);

    PLASMA_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).m_Result.Succeeded());
    PLASMA_TEST_BOOL(manager.CanAdd(ExtendedOuterClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).m_Result.Succeeded());
    PLASMA_TEST_BOOL(!manager.CanAdd(plReflectedClass::GetStaticRTTI(), pObject, "ClassPtrArray", 0).m_Result.Succeeded());

    for (plInt32 i = 0; i < PLASMA_ARRAY_SIZE(pChildren); i++)
    {
      PLASMA_TEST_BOOL(manager.CanAdd(plObjectTest::GetStaticRTTI(), pChildObject, szProperty, i).m_Result.Succeeded());
      pChildren[i] = manager.CreateObject(plObjectTest::GetStaticRTTI());
      manager.AddObject(pChildren[i], pChildObject, szProperty, i);
      PLASMA_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }
    PLASMA_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 4);

    PLASMA_TEST_BOOL_MSG(manager.CanMove(pObject, pChildObject, szProperty, 0).m_Result.Failed(), "Can't move to own child");
    PLASMA_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 1).m_Result.Failed(), "Can't move before onself");
    PLASMA_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildObject, szProperty, 2).m_Result.Failed(), "Can't move after oneself");
    PLASMA_TEST_BOOL_MSG(manager.CanMove(pChildren[1], pChildren[1], szProperty, 0).m_Result.Failed(), "Can't move into yourself");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DocumentSubElementObject")
  {
    const char* szProperty = "ClassArray";
    for (plInt32 i = 0; i < PLASMA_ARRAY_SIZE(pSubElementObject); i++)
    {
      PLASMA_TEST_BOOL(manager.CanAdd(OuterClass::GetStaticRTTI(), pObject, szProperty, i).m_Result.Succeeded());
      pSubElementObject[i] = manager.CreateObject(OuterClass::GetStaticRTTI());
      manager.AddObject(pSubElementObject[i], pObject, szProperty, i);
      PLASMA_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), i + 1);
    }

    PLASMA_TEST_BOOL(manager.CanRemove(pSubElementObject[0]).m_Result.Succeeded());
    manager.RemoveObject(pSubElementObject[0]);
    manager.DestroyObject(pSubElementObject[0]);
    pSubElementObject[0] = nullptr;
    PLASMA_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 3);

    plVariant value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[1]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[3]->GetGuid());

    PLASMA_TEST_BOOL(manager.CanMove(pSubElementObject[1], pObject, szProperty, 2).m_Result.Succeeded());
    manager.MoveObject(pSubElementObject[1], pObject, szProperty, 2);
    PLASMA_TEST_BOOL(manager.CanMove(pSubElementObject[3], pObject, szProperty, 0).m_Result.Succeeded());
    manager.MoveObject(pSubElementObject[3], pObject, szProperty, 0);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[3]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 2);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[1]->GetGuid());

    PLASMA_TEST_BOOL(manager.CanRemove(pSubElementObject[3]).m_Result.Succeeded());
    manager.RemoveObject(pSubElementObject[3]);
    manager.DestroyObject(pSubElementObject[3]);
    pSubElementObject[3] = nullptr;
    PLASMA_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pObject->GetTypeAccessor().GetValue(szProperty, 0);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[2]->GetGuid());
    value = pObject->GetTypeAccessor().GetValue(szProperty, 1);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[1]->GetGuid());

    PLASMA_TEST_BOOL(manager.CanMove(pSubElementObject[1], pChildObject, szProperty, 0).m_Result.Succeeded());
    manager.MoveObject(pSubElementObject[1], pChildObject, szProperty, 0);
    PLASMA_TEST_BOOL(manager.CanMove(pSubElementObject[2], pChildObject, szProperty, 0).m_Result.Succeeded());
    manager.MoveObject(pSubElementObject[2], pChildObject, szProperty, 0);

    PLASMA_TEST_INT(pObject->GetTypeAccessor().GetCount(szProperty), 0);
    PLASMA_TEST_INT(pChildObject->GetTypeAccessor().GetCount(szProperty), 2);

    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 0);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[2]->GetGuid());
    value = pChildObject->GetTypeAccessor().GetValue(szProperty, 1);
    PLASMA_TEST_BOOL(value.IsA<plUuid>() && value.Get<plUuid>() == pSubElementObject[1]->GetGuid());
  }

  manager.DestroyAllObjects();
}
