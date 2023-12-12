#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects
  {
    struct
    {
      plGameObject* pParent1;
      plGameObject* pParent2;
      plGameObject* pChild11;
      plGameObject* pChild21;
    };
    plGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(plWorld& ref_world, bool bDynamic)
  {
    TestWorldObjects testWorldObjects;
    plMemoryUtils::ZeroFill(&testWorldObjects, 1);

    plQuat q;
    q.SetFromAxisAndAngle(plVec3(0.0f, 0.0f, 1.0f), plAngle::Degree(90.0f));

    plGameObjectDesc desc;
    desc.m_bDynamic = bDynamic;
    desc.m_LocalPosition = plVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = plVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    ref_world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    ref_world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_hParent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    ref_world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_hParent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    ref_world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o, plVec3 vOffset = plVec3(100.0f, 0.0f, 0.0f))
  {
    const float eps = plMath::DefaultEpsilon<float>();
    plQuat q;
    q.SetFromAxisAndAngle(plVec3(0.0f, 0.0f, 1.0f), plAngle::Degree(90.0f));

    for (plUInt32 i = 0; i < 2; ++i)
    {
      PLASMA_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset, 0);
      PLASMA_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      PLASMA_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), plVec3(1.5f, 1.5f, 1.5f), 0);
    }

    for (plUInt32 i = 2; i < 4; ++i)
    {
      PLASMA_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), vOffset + plVec3(0.0f, 150.0f, 0.0f), eps * 2.0f);
      PLASMA_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q * q, eps * 10.0f));
      PLASMA_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), plVec3(2.25f, 2.25f, 2.25f), 0);
    }
  }

  void SanityCheckWorld(plWorld& ref_world)
  {
    struct Traverser
    {
      Traverser(plWorld& ref_world)
        : m_World(ref_world)
      {
      }

      plWorld& m_World;
      plSet<plGameObject*> m_Found;

      plVisitorExecution::Enum Visit(plGameObject* pObject)
      {
        plGameObject* pObject2 = nullptr;
        PLASMA_TEST_BOOL_MSG(m_World.TryGetObject(pObject->GetHandle(), pObject2), "Visited object that is not part of the world!");
        PLASMA_TEST_BOOL_MSG(pObject2 == pObject, "Handle did not resolve to the same object!");
        PLASMA_TEST_BOOL_MSG(!m_Found.Contains(pObject), "Object visited twice!");
        m_Found.Insert(pObject);

        const plUInt32 uiChildren = pObject->GetChildCount();
        plUInt32 uiChildren2 = 0;
        for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
        {
          uiChildren2++;
          auto handle = it->GetHandle();
          plGameObject* pChild = nullptr;
          PLASMA_TEST_BOOL_MSG(m_World.TryGetObject(handle, pChild), "Could not resolve child!");
          plGameObject* pParent = pChild->GetParent();
          PLASMA_TEST_BOOL_MSG(pParent == pObject, "pObject's child's parent does not point to pObject!");
        }
        PLASMA_TEST_INT(uiChildren, uiChildren2);
        return plVisitorExecution::Continue;
      }
    };

    Traverser traverser(ref_world);
    ref_world.Traverse(plWorld::VisitorFunc(&Traverser::Visit, &traverser), plWorld::TraversalMethod::BreadthFirst);
  }

  class CustomCoordinateSystemProvider : public plCoordinateSystemProvider
  {
  public:
    CustomCoordinateSystemProvider(const plWorld* pWorld)
      : plCoordinateSystemProvider(pWorld)
    {
    }

    virtual void GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_coordinateSystem) const override
    {
      const plMat3 mTmp = plGraphicsUtils::CreateLookAtViewMatrix(-vGlobalPosition, plVec3(0, 0, 1), plHandedness::LeftHanded);

      out_coordinateSystem.m_vRightDir = mTmp.GetRow(0);
      out_coordinateSystem.m_vUpDir = mTmp.GetRow(1);
      out_coordinateSystem.m_vForwardDir = mTmp.GetRow(2);
    }
  };

  class VelocityTestModule : public plWorldModule
  {
    PLASMA_ADD_DYNAMIC_REFLECTION(VelocityTestModule, plWorldModule);
    PLASMA_DECLARE_WORLD_MODULE();

  public:
    VelocityTestModule(plWorld* pWorld)
      : plWorldModule(pWorld)
    {
    }

    virtual void Initialize() override
    {
      {
        auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::SetLocalPos, this);
        RegisterUpdateFunction(desc);
      }

      {
        auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(VelocityTestModule::ResetGlobalPos, this);
        RegisterUpdateFunction(desc);
      }
    }

    void SetLocalPos(const UpdateContext&)
    {
      if (m_bSetLocalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        plUInt32 i = it->GetHandle().GetInternalID().m_InstanceIndex;

        plVec3 newPos = plVec3(i * 10, 0, 0);
        it->SetLocalPosition(newPos);

        plQuat newRot;
        newRot.SetFromAxisAndAngle(plVec3::UnitZAxis(), plAngle::Degree(i * 30));
        it->SetLocalRotation(newRot);

        if (i > 5)
        {
          it->UpdateGlobalTransform();
        }
        if (i > 8)
        {
          it->UpdateGlobalTransformAndBounds();
        }
      }
    }

    void ResetGlobalPos(const UpdateContext&)
    {
      if (m_bResetGlobalPos == false)
        return;

      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SetGlobalPosition(plVec3::ZeroVector());
      }
    }

    bool m_bSetLocalPos = false;
    bool m_bResetGlobalPos = false;
  };

  // clang-format off
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(VelocityTestModule, 1, plRTTINoAllocator)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
  PLASMA_IMPLEMENT_WORLD_MODULE(VelocityTestModule);
  // clang-format on
} // namespace

class plGameObjectTest
{
public:
  static void TestInternals(plGameObject* pObject, plGameObject* pParent, plUInt32 uiHierarchyLevel)
  {
    PLASMA_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    PLASMA_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);

    if (pParent)
    {
      PLASMA_TEST_BOOL(pObject->m_pTransformationData->m_pParentData->m_pObject == pParent);
    }

    PLASMA_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    PLASMA_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

PLASMA_CREATE_SIMPLE_TEST(World, World)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transforms dynamic")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    plVec3 offset = plVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    world.Update();

    TestTransforms(o, offset);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transforms static")
  {
    plWorldDesc worldDesc("Test");
    worldDesc.m_bReportErrorWhenStaticObjectMoves = false;

    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    plVec3 offset = plVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    // No need to call world update since global transform is updated immediately for static objects.
    // world.Update();

    TestTransforms(o, offset);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GameObject parenting")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    const float eps = plMath::DefaultEpsilon<float>();
    plQuat q;
    q.SetFromAxisAndAngle(plVec3(0.0f, 0.0f, 1.0f), plAngle::Degree(90.0f));

    plGameObjectDesc desc;
    desc.m_LocalPosition = plVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = plVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    plGameObject* pParentObject;
    plGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    PLASMA_TEST_VEC3(pParentObject->GetLocalPosition(), desc.m_LocalPosition, 0);
    PLASMA_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    PLASMA_TEST_VEC3(pParentObject->GetLocalScaling(), desc.m_LocalScaling, 0);

    PLASMA_TEST_VEC3(pParentObject->GetGlobalPosition(), desc.m_LocalPosition, 0);
    PLASMA_TEST_BOOL(pParentObject->GetGlobalRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    PLASMA_TEST_VEC3(pParentObject->GetGlobalScaling(), desc.m_LocalScaling, 0);

    PLASMA_TEST_BOOL(pParentObject->GetName() == desc.m_sName.GetString());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_hParent = parentObject;

    plGameObjectHandle childObjects[10];
    for (plUInt32 i = 0; i < 10; ++i)
    {
      plStringBuilder sb;
      sb.AppendFormat("Child_{0}", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = plVec3(i * 10.0f, 0.0f, 0.0f);

      childObjects[i] = world.CreateObject(desc);
    }

    plUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      plStringBuilder sb;
      sb.AppendFormat("Child_{0}", uiCounter);

      PLASMA_TEST_BOOL(it->GetName() == sb);

      PLASMA_TEST_VEC3(it->GetGlobalPosition(), plVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      PLASMA_TEST_BOOL(it->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      PLASMA_TEST_VEC3(it->GetGlobalScaling(), plVec3(1.5f, 1.5f, 1.5f), 0.0f);

      ++uiCounter;
    }

    PLASMA_TEST_INT(uiCounter, 10);
    PLASMA_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObjectNow(childObjects[0]);
    world.DeleteObjectNow(childObjects[3]);
    world.DeleteObjectNow(childObjects[9]);

    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    plUInt32 indices[7] = {1, 2, 4, 5, 6, 7, 8};

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      plStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      PLASMA_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    PLASMA_TEST_INT(uiCounter, 7);
    PLASMA_TEST_INT(pParentObject->GetChildCount(), 7);

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      plStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      PLASMA_TEST_BOOL(it->GetName() == sb);

      ++uiCounter;
    }

    PLASMA_TEST_INT(uiCounter, 7);
    PLASMA_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObjectDelayed(parentObject);
    PLASMA_TEST_BOOL(world.IsValidObject(parentObject));

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(indices); ++i)
    {
      PLASMA_TEST_BOOL(world.IsValidObject(childObjects[indices[i]]));
    }

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    PLASMA_TEST_BOOL(!world.IsValidObject(parentObject));

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    PLASMA_TEST_INT(world.GetObjectCount(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Re-parenting 1")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    plGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    plGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1);
    plGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    plGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2);

    PLASMA_TEST_INT(o.pParent1->GetChildCount(), 2);
    auto it = o.pParent1->GetChildren();
    PLASMA_TEST_BOOL(o.pChild11 == it);
    ++it;
    PLASMA_TEST_BOOL(o.pParent2 == it);
    ++it;
    PLASMA_TEST_BOOL(!it.IsValid());

    it = o.pParent2->GetChildren();
    PLASMA_TEST_BOOL(o.pChild21 == it);
    ++it;
    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Re-parenting 2")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pChild21->SetParent(plGameObjectHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    plGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    plGameObjectTest::TestInternals(o.pParent2, nullptr, 0);
    plGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    plGameObjectTest::TestInternals(o.pChild21, nullptr, 0);

    auto it = o.pParent1->GetChildren();
    PLASMA_TEST_BOOL(o.pChild11 == it);
    ++it;
    PLASMA_TEST_BOOL(!it.IsValid());

    PLASMA_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Re-parenting 3")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);
    SanityCheckWorld(world);
    // Here we test whether the sibling information is correctly cleared.

    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling.
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) and next (pParent2) sibling.
    o.pChild21->SetParent(plGameObjectHandle());
    SanityCheckWorld(world);
    // pChild21 has no siblings.
    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling again.
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Traversal")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        plVisitorExecution::Enum Visit(plGameObject* pObject)
        {
          if (m_uiCounter < PLASMA_ARRAY_SIZE(m_o.pObjects))
          {
            PLASMA_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return plVisitorExecution::Continue;
        }

        plUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      BreadthFirstTest bft;
      bft.m_o = o;

      world.Traverse(plWorld::VisitorFunc(&BreadthFirstTest::Visit, &bft), plWorld::BreadthFirst);
      PLASMA_TEST_INT(bft.m_uiCounter, PLASMA_ARRAY_SIZE(o.pObjects));
    }

    {
      world.CreateObject(plGameObjectDesc());

      struct DepthFirstTest
      {
        DepthFirstTest() { m_uiCounter = 0; }

        plVisitorExecution::Enum Visit(plGameObject* pObject)
        {
          if (m_uiCounter == 0)
          {
            PLASMA_TEST_BOOL(pObject == m_o.pParent1);
          }
          else if (m_uiCounter == 1)
          {
            PLASMA_TEST_BOOL(pObject == m_o.pChild11);
          }
          else if (m_uiCounter == 2)
          {
            PLASMA_TEST_BOOL(pObject == m_o.pParent2);
          }
          else if (m_uiCounter == 3)
          {
            PLASMA_TEST_BOOL(pObject == m_o.pChild21);
          }

          ++m_uiCounter;
          if (m_uiCounter >= PLASMA_ARRAY_SIZE(m_o.pObjects))
            return plVisitorExecution::Stop;

          return plVisitorExecution::Continue;
        }

        plUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      DepthFirstTest dft;
      dft.m_o = o;

      world.Traverse(plWorld::VisitorFunc(&DepthFirstTest::Visit, &dft), plWorld::DepthFirst);
      PLASMA_TEST_INT(dft.m_uiCounter, PLASMA_ARRAY_SIZE(o.pObjects));
    }

    {
      PLASMA_TEST_INT(world.GetObjectCount(), 5);
      world.DeleteObjectNow(o.pChild11->GetHandle(), false);
      PLASMA_TEST_INT(world.GetObjectCount(), 4);

      for (auto it = world.GetObjects(); it.IsValid(); ++it)
      {
        PLASMA_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }

      const plWorld& constWorld = world;
      for (auto it = constWorld.GetObjects(); it.IsValid(); ++it)
      {
        PLASMA_TEST_BOOL(!it->GetHandle().IsInvalidated());
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Multiple Worlds")
  {
    plWorldDesc worldDesc1("Test1");
    plWorld world1(worldDesc1);
    PLASMA_LOCK(world1.GetWriteMarker());

    plWorldDesc worldDesc2("Test2");
    plWorld world2(worldDesc2);
    PLASMA_LOCK(world2.GetWriteMarker());

    plGameObjectDesc desc;
    desc.m_sName.Assign("Obj1");

    plGameObjectHandle hObj1 = world1.CreateObject(desc);
    PLASMA_TEST_BOOL(world1.IsValidObject(hObj1));

    desc.m_sName.Assign("Obj2");

    plGameObjectHandle hObj2 = world2.CreateObject(desc);
    PLASMA_TEST_BOOL(world2.IsValidObject(hObj2));

    plGameObject* pObj1 = nullptr;
    PLASMA_TEST_BOOL(world1.TryGetObject(hObj1, pObj1));
    PLASMA_TEST_BOOL(pObj1 != nullptr);

    pObj1->SetGlobalKey("Obj1");
    pObj1 = nullptr;
    PLASMA_TEST_BOOL(world1.TryGetObjectWithGlobalKey(plTempHashedString("Obj1"), pObj1));
    PLASMA_TEST_BOOL(!world1.TryGetObjectWithGlobalKey(plTempHashedString("Obj2"), pObj1));
    PLASMA_TEST_BOOL(pObj1 != nullptr);

    plGameObject* pObj2 = nullptr;
    PLASMA_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    PLASMA_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Obj2");
    pObj2 = nullptr;
    PLASMA_TEST_BOOL(world2.TryGetObjectWithGlobalKey(plTempHashedString("Obj2"), pObj2));
    PLASMA_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(plTempHashedString("Obj1"), pObj2));
    PLASMA_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Deschd");
    PLASMA_TEST_BOOL(world2.TryGetObjectWithGlobalKey(plTempHashedString("Deschd"), pObj2));
    PLASMA_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(plTempHashedString("Obj2"), pObj2));

    world2.DeleteObjectNow(hObj2);

    PLASMA_TEST_BOOL(!world2.IsValidObject(hObj2));
    PLASMA_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(plTempHashedString("Deschd"), pObj2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Custom coordinate system")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);

    plSharedPtr<CustomCoordinateSystemProvider> pProvider = PLASMA_DEFAULT_NEW(CustomCoordinateSystemProvider, &world);
    CustomCoordinateSystemProvider* pProviderBackup = pProvider.Borrow();

    world.SetCoordinateSystemProvider(pProvider);
    PLASMA_TEST_BOOL(&world.GetCoordinateSystemProvider() == pProviderBackup);

    plVec3 pos = plVec3(2, 3, 0);

    plCoordinateSystem coordSys;
    world.GetCoordinateSystem(pos, coordSys);

    PLASMA_TEST_VEC3(coordSys.m_vForwardDir, (-pos).GetNormalized(), plMath::SmallEpsilon<float>());
    PLASMA_TEST_VEC3(coordSys.m_vUpDir, plVec3(0, 0, 1), plMath::SmallEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Active Flag / Active State")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    PLASMA_LOCK(world.GetWriteMarker());

    plGameObjectHandle hParent;
    plGameObjectDesc desc;
    plGameObjectHandle hObjects[10];
    plGameObject* pObjects[10];

    for (plUInt32 i = 0; i < 10; ++i)
    {
      desc.m_hParent = hParent;
      hObjects[i] = world.CreateObject(desc, pObjects[i]);
      hParent = hObjects[i];

      PLASMA_TEST_BOOL(pObjects[i]->GetActiveFlag());
      PLASMA_TEST_BOOL(pObjects[i]->IsActive());
    }

    plUInt32 iTopDisabled = 1;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      PLASMA_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(pObjects[i]->GetActiveFlag() == true);
      PLASMA_TEST_BOOL(pObjects[i]->IsActive() == true);
    }

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(pObjects[i]->GetActiveFlag() == (i != iTopDisabled));
      PLASMA_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    iTopDisabled = 3;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }

    pObjects[iTopDisabled]->SetActiveFlag(true);

    iTopDisabled = 5;
    pObjects[iTopDisabled]->SetActiveFlag(false);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(pObjects[i]->IsActive() == (i < iTopDisabled));
    }
  }
}
