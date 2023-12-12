#include <GameEngineTest/GameEngineTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Utilities/DataStructures/DynamicOctree.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(DataStructures);

namespace DynamicOctreeTestDetail
{
  static plInt32 g_iSearchInstance = 0;
  static bool g_bFoundSearched = false;
  static plUInt32 g_iReturned = 0;

  static bool ObjectFound(void* pPassThrough, plDynamicTreeObjectConst object)
  {
    PLASMA_TEST_BOOL(pPassThrough == nullptr);

    ++g_iReturned;

    if (object.Value().m_iObjectInstance == g_iSearchInstance)
      g_bFoundSearched = true;

    // let it give us all the objects in range and count how many that are
    return true;
  }
} // namespace DynamicOctreeTestDetail

PLASMA_CREATE_SIMPLE_TEST(DataStructures, DynamicOctree)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CreateTree / GetBoundingBox")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3(100, 200, 300), plVec3(300, 400, 500), 1.0f);

    const plBoundingBox& bb = o.GetBoundingBox();

    PLASMA_TEST_VEC3(bb.GetCenter(), plVec3(100, 200, 300), 0.01f);
    PLASMA_TEST_VEC3(bb.GetHalfExtents(), plVec3(500), 0.01f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert Inside / Outside")
  {
    const plVec3 c(100, 200, 300);
    const float e = 50;

    plDynamicOctree o;
    o.CreateTree(c, plVec3(e), 1.0f);
    plInt32 iInstance = 0;

    for (float z = -e - 99; z < e + 100; z += 10.0f)
    {
      for (float y = -e - 99; y < e + 100; y += 10.0f)
      {
        for (float x = -e - 99; x < e + 100; x += 10.0f)
        {
          const bool bInside = (z > -e) && (z < e) && (y > -e) && (y < e) && (x > -e) && (x < e);

          PLASMA_TEST_BOOL(o.InsertObject(c + plVec3(x, y, z), plVec3(1.0f), 0, iInstance, nullptr, true) == (bInside ? PLASMA_SUCCESS : PLASMA_FAILURE));
          PLASMA_TEST_BOOL(o.InsertObject(c + plVec3(x, y, z), plVec3(1.0f), 0, iInstance, nullptr, false) == PLASMA_SUCCESS);

          ++iInstance;
        }
      }
    }
  }

  struct TestObject
  {
    plVec3 m_vPos;
    plVec3 m_vExtents;
    plDynamicTreeObject m_hObject;
  };

  plDeque<TestObject> Objects;

  {
    TestObject to;


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 0);
    to.m_vExtents.Set(2.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(-90, 50, 80);
    to.m_vExtents.Set(2.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(0, 0, -50);
    to.m_vExtents.Set(20.0f, 4.0f, 10.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(10, 10, 10);
    to.m_vExtents.Set(50.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);


    to.m_vPos.Set(50, -20, 10);
    to.m_vExtents.Set(1.0f, 2.0f, 1.0f);

    Objects.PushBack(to);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindObjectsInRange(Point)")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindObjectsInRange(Radius)")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      // point inside object

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents * 0.9f, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      // point outside object

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos + Objects[i].m_vExtents + plVec3(2, 0, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());

      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos - Objects[i].m_vExtents - plVec3(0, 2, 0), 2.5f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == true);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_iReturned < Objects.GetCount());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveObject(handle)")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(Objects[i].m_hObject);

      // one less in the tree
      PLASMA_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveObject(index)")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i;

      o.RemoveObject(i, i + 1);

      // one less in the tree
      PLASMA_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveObjectsOfType")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      DynamicOctreeTestDetail::g_iSearchInstance = i + 1;

      o.RemoveObjectsOfType(i);

      // one less in the tree
      PLASMA_TEST_INT(o.GetCount(), Objects.GetCount() - i - 1);

      // searching for it, won't return it anymore
      DynamicOctreeTestDetail::g_iReturned = 0;
      DynamicOctreeTestDetail::g_bFoundSearched = false;
      o.FindObjectsInRange(Objects[i].m_vPos, 1.0f, DynamicOctreeTestDetail::ObjectFound, nullptr);
      PLASMA_TEST_BOOL(DynamicOctreeTestDetail::g_bFoundSearched == false);
    }

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, 0, i, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveObjectsOfType(0);

    PLASMA_TEST_BOOL(o.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAllObjects")
  {
    plDynamicOctree o;
    o.CreateTree(plVec3::ZeroVector(), plVec3(100), 1.0f);

    for (plUInt32 i = 0; i < Objects.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(o.InsertObject(Objects[i].m_vPos, Objects[i].m_vExtents, i, i + 1, &Objects[i].m_hObject, false) == PLASMA_SUCCESS);

      PLASMA_TEST_BOOL(o.IsEmpty() == false);
      PLASMA_TEST_INT(o.GetCount(), i + 1);
    }

    o.RemoveAllObjects();
    PLASMA_TEST_BOOL(o.IsEmpty());
    PLASMA_TEST_INT(o.GetCount(), 0);
  }
}
