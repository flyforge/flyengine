#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  class plTestComponentManager;

  class plTestComponent : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(plTestComponent, plComponent, plTestComponentManager);
  };

  class plTestComponentManager : public plComponentManager<class plTestComponent, plBlockStorageType::FreeList>
  {
  public:
    plTestComponentManager(plWorld* pWorld)
      : plComponentManager<plTestComponent, plBlockStorageType::FreeList>(pWorld)
    {
      m_qRotation.SetIdentity();
    }

    virtual void Initialize() override
    {
      auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&plTestComponentManager::Update, this), "Update");
      desc.m_bOnlyUpdateWhenSimulating = false;

      RegisterUpdateFunction(desc);
    }

    void Update(const plWorldModule::UpdateContext& context)
    {
      plQuat qRot;
      qRot.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(2.0f));

      m_qRotation = qRot * m_qRotation;

      for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
      {
        ComponentType* pComponent = it;
        if (pComponent->IsActiveAndInitialized())
        {
          auto pOwner = pComponent->GetOwner();
          pOwner->SetLocalRotation(m_qRotation);
        }
      }
    }

    plQuat m_qRotation;
  };

  // clang-format off
  PLASMA_BEGIN_COMPONENT_TYPE(plTestComponent, 1, plComponentMode::Dynamic);
  PLASMA_END_COMPONENT_TYPE;
  // clang-format on

  void AddObjectsToWorld(plWorld& ref_world, bool bDynamic, plUInt32 uiNumObjects, plUInt32 uiTreeLevelNumNodeDiv, plUInt32 uiTreeDepth,
    plInt32 iAttachCompsDepth, plGameObjectHandle hParent = plGameObjectHandle())
  {
    if (uiTreeDepth == 0)
      return;

    plGameObjectDesc gd;
    gd.m_bDynamic = bDynamic;
    gd.m_hParent = hParent;

    float posX = 0.0f;
    float posY = uiTreeDepth * 5.0f;

    plTestComponentManager* pMan = ref_world.GetOrCreateComponentManager<plTestComponentManager>();

    for (plUInt32 i = 0; i < uiNumObjects; ++i)
    {
      gd.m_LocalPosition.Set(posX, posY, 0);
      posX += 5.0f;

      plGameObject* pObj;
      auto hObj = ref_world.CreateObject(gd, pObj);

      if (iAttachCompsDepth > 0)
      {
        plTestComponent* comp;
        pMan->CreateComponent(pObj, comp);
      }

      AddObjectsToWorld(
        ref_world, bDynamic, plMath::Max(uiNumObjects / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(
    bool bDynamic, plUInt32 uiNumObjects, plUInt32 uiTreeLevelNumNodeDiv, plUInt32 uiTreeDepth, plInt32 iAttachCompsDepth, plWorld* pWorld = nullptr)
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);

    if (pWorld == nullptr)
    {
      pWorld = &world;
    }

    PLASMA_LOCK(pWorld->GetWriteMarker());

    {
      plStopwatch sw;

      AddObjectsToWorld(*pWorld, bDynamic, uiNumObjects, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Creating %u %s objects (depth: %u): %.2fms", pWorld->GetObjectCount(),
        bDynamic ? "dynamic" : "static", uiTreeDepth, tDiff.GetMilliseconds());
    }
  }

} // namespace


#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
static const plTestBlock::Enum EnableInRelease = plTestBlock::DisabledNoWarning;
#else
static const plTestBlock::Enum EnableInRelease = plTestBlock::Enabled;
#endif

PLASMA_CREATE_SIMPLE_TEST(World, Profile_Creation)
{
  PLASMA_TEST_BLOCK(EnableInRelease, "Create many objects")
  {
    // it makes no difference whether we create static or dynamic objects
    static bool bDynamic = true;
    bDynamic = !bDynamic;

    MeasureCreationTime(bDynamic, 10, 1, 4, 0);
    MeasureCreationTime(bDynamic, 10, 1, 5, 0);
    MeasureCreationTime(bDynamic, 100, 1, 2, 0);
    MeasureCreationTime(bDynamic, 10000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 1000000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100, 1, 3, 0);
    MeasureCreationTime(bDynamic, 3, 1, 12, 0);
    MeasureCreationTime(bDynamic, 1, 1, 80, 0);
  }
}

PLASMA_CREATE_SIMPLE_TEST(World, Profile_Deletion)
{
  PLASMA_TEST_BLOCK(EnableInRelease, "Delete many objects")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    plStopwatch sw;

    PLASMA_LOCK(world.GetWriteMarker());
    plUInt32 uiNumObjects = world.GetObjectCount();

    world.Clear();
    world.Update();

    const plTime tDiff = sw.Checkpoint();
    plTestFramework::Output(plTestOutput::Duration, "Deleting %u objects: %.2fms", uiNumObjects, tDiff.GetMilliseconds());
  }
}

PLASMA_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  PLASMA_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static objects")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    MeasureCreationTime(false, 100, 1, 3, 0, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  PLASMA_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 0, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  PLASMA_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects with components")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  PLASMA_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic objects")
  {
    plWorldDesc worldDesc("Test");
    plWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  PLASMA_TEST_BLOCK(EnableInRelease, "MT Update 250,000 dynamic objects")
  {
    plWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    plWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  PLASMA_TEST_BLOCK(EnableInRelease, "MT Update 1,000,000 dynamic objects")
  {
    plWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    plWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    plStopwatch sw;

    // first round always has some overhead
    for (plUInt32 i = 0; i < 3; ++i)
    {
      PLASMA_LOCK(world.GetWriteMarker());
      world.Update();

      const plTime tDiff = sw.Checkpoint();

      plTestFramework::Output(plTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
