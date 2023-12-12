#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>


PLASMA_CREATE_SIMPLE_TEST(DocumentObject, ObjectMetaData)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Pointers / int")
  {
    plObjectMetaData<void*, plInt32> meta;

    int a = 0, b = 1, c = 2, d = 3;

    PLASMA_TEST_BOOL(!meta.HasMetaData(&a));
    PLASMA_TEST_BOOL(!meta.HasMetaData(&b));
    PLASMA_TEST_BOOL(!meta.HasMetaData(&c));
    PLASMA_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pData = meta.BeginModifyMetaData(&a);
      *pData = a;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&b);
      *pData = b;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&c);
      *pData = c;
      meta.EndModifyMetaData();
    }

    PLASMA_TEST_BOOL(meta.HasMetaData(&a));
    PLASMA_TEST_BOOL(meta.HasMetaData(&b));
    PLASMA_TEST_BOOL(meta.HasMetaData(&c));
    PLASMA_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pDataR = meta.BeginReadMetaData(&a);
      PLASMA_TEST_INT(*pDataR, a);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&b);
      PLASMA_TEST_INT(*pDataR, b);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&c);
      PLASMA_TEST_INT(*pDataR, c);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&d);
      PLASMA_TEST_INT(*pDataR, 0);
      meta.EndReadMetaData();
    }
  }

  struct md
  {
    md() { b = false; }

    plString s;
    bool b;
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UUID / struct")
  {
    plObjectMetaData<plUuid, md> meta;

    const int num = 100;

    plDynamicArray<plUuid> obj;
    obj.SetCount(num);

    for (plUInt32 i = 0; i < num; ++i)
    {
      plUuid& uid = obj[i];
      uid.CreateNewUuid();

      if (plMath::IsEven(i))
      {
        auto d1 = meta.BeginModifyMetaData(uid);
        d1->b = true;
        d1->s = "test";

        meta.EndModifyMetaData();
      }

      PLASMA_TEST_BOOL(meta.HasMetaData(uid) == plMath::IsEven(i));
    }

    for (plUInt32 i = 0; i < num; ++i)
    {
      const plUuid& uid = obj[i];

      auto p = meta.BeginReadMetaData(uid);

      PLASMA_TEST_BOOL(p->b == plMath::IsEven(i));

      if (plMath::IsEven(i))
      {
        PLASMA_TEST_STRING(p->s, "test");
      }
      else
      {
        PLASMA_TEST_BOOL(p->s.IsEmpty());
      }

      meta.EndReadMetaData();
      PLASMA_TEST_BOOL(meta.HasMetaData(uid) == plMath::IsEven(i));
    }
  }
}
