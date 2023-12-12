#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static plSpatialData::Category s_SpecialTestCategory = plSpatialData::RegisterCategory("SpecialTestCategory", plSpatialData::Flags::None);

  using TestBoundsComponentManager = plComponentManager<class TestBoundsComponent, plBlockStorageType::Compact>;

  class TestBoundsComponent : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestBoundsComponent, plComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      plBoundingBox bounds;
      bounds.SetCenterAndHalfExtents(plVec3::ZeroVector(), plVec3(x, y, z));

      plSpatialData::Category category = m_SpecialCategory;
      if (category == plInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(bounds, category);
    }

    plSpatialData::Category m_SpecialCategory = plInvalidSpatialDataCategory;
  };

  // clang-format off
  PLASMA_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1, plComponentMode::Static)
  {
    PLASMA_BEGIN_MESSAGEHANDLERS
    {
      PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
    }
    PLASMA_END_MESSAGEHANDLERS;
  }
  PLASMA_END_COMPONENT_TYPE;
  // clang-format on
} // namespace

PLASMA_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  plWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  plWorld world(worldDesc);
  PLASMA_LOCK(world.GetWriteMarker());

  auto& rng = world.GetRandomNumberGenerator();

  plDynamicArray<plGameObject*> objects;
  objects.Reserve(1000);

  for (plUInt32 i = 0; i < 1000; ++i)
  {
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    plGameObjectDesc desc;
    desc.m_bDynamic = (i >= 500);
    desc.m_LocalPosition = plVec3(x, y, z);

    plGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    objects.PushBack(pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);
  }

  world.Update();

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindObjectsInSphere")
  {
    plBoundingSphere testSphere(plVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    plDynamicArray<plGameObject*> objectsInSphere;
    plHashSet<plGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      plBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      PLASMA_TEST_BOOL(testSphere.Overlaps(objSphere));
      PLASMA_TEST_BOOL(!uniqueObjects.Insert(pObject));
      PLASMA_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      plBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        PLASMA_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((plGameObject*)it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, [&](plGameObject* pObject) {
      objectsInSphere.PushBack(pObject);
      PLASMA_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return plVisitorExecution::Continue; });

    for (auto pObject : objectsInSphere)
    {
      plBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      PLASMA_TEST_BOOL(testSphere.Overlaps(objSphere));
      PLASMA_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      plBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        PLASMA_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((plGameObject*)it));
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindObjectsInBox")
  {
    plBoundingBox testBox;
    testBox.SetCenterAndHalfExtents(plVec3(100.0f, 60.0f, 400.0f), plVec3(3000.0f));

    plDynamicArray<plGameObject*> objectsInBox;
    plHashSet<plGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      plBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      PLASMA_TEST_BOOL(testBox.Overlaps(objBox));
      PLASMA_TEST_BOOL(!uniqueObjects.Insert(pObject));
      PLASMA_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      plBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        PLASMA_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((plGameObject*)it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, [&](plGameObject* pObject) {
      objectsInBox.PushBack(pObject);
      PLASMA_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return plVisitorExecution::Continue; });

    for (auto pObject : objectsInBox)
    {
      plBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      PLASMA_TEST_BOOL(testBox.Overlaps(objSphere));
      PLASMA_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      plBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        PLASMA_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((plGameObject*)it));
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindVisibleObjects")
  {
    constexpr uint32_t numUpdates = 13;

    // update a few times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    plMat4 lookAt = plGraphicsUtils::CreateLookAtViewMatrix(plVec3::ZeroVector(), plVec3::UnitXAxis(), plVec3::UnitZAxis());
    plMat4 projection = plGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(plAngle::Degree(80.0f), 1.0f, 1.0f, 10000.0f);

    plFrustum testFrustum;
    testFrustum.SetFrustum(projection * lookAt);

    plDynamicArray<const plGameObject*> visibleObjects;
    plHashSet<const plGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {}, plVisibilityState::Direct);

    PLASMA_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      PLASMA_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      PLASMA_TEST_BOOL(!uniqueObjects.Insert(pObject));
      PLASMA_TEST_BOOL(pObject->IsDynamic());

      plVisibilityState visType = pObject->GetVisibilityState();
      PLASMA_TEST_BOOL(visType == plVisibilityState::Direct);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      plGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == plVolumePosition::Outside)
      {
        plVisibilityState visType = pObject->GetVisibilityState();
        PLASMA_TEST_BOOL(visType == plVisibilityState::Invisible);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        plVec3 pos = it->GetLocalPosition();

        pos.x += (float)rng.DoubleMinMax(-range, range);
        pos.y += (float)rng.DoubleMinMax(-range, range);
        pos.z += (float)rng.DoubleMinMax(-range, range);

        it->SetLocalPosition(pos);
      }
    }

    world.Update();

    // Check that last frame visible doesn't reset entirely after moving
    for (const plGameObject* pObject : visibleObjects)
    {
      plVisibilityState visType = pObject->GetVisibilityState();
      PLASMA_TEST_BOOL(visType == plVisibilityState::Direct);
    }
  }

  if (false)
  {
    plStringBuilder outputPath = plTestFramework::GetInstance()->GetAbsOutputPath();
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

    plFileWriter fileWriter;
    if (fileWriter.Open(":output/profiling.json") == PLASMA_SUCCESS)
    {
      plProfilingSystem::ProfilingData profilingData;
      plProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      plLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }

  // Test multiple categories for spatial data
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MultipleCategories")
  {
    for (plUInt32 i = 0; i < objects.GetCount(); ++i)
    {
      plGameObject* pObject = objects[i];

      TestBoundsComponent* pComponent = nullptr;
      TestBoundsComponent::CreateComponent(pObject, pComponent);
      pComponent->m_SpecialCategory = s_SpecialTestCategory;
    }

    world.Update();

    plDynamicArray<plGameObjectHandle> allObjects;
    allObjects.Reserve(world.GetObjectCount());

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      allObjects.PushBack(it->GetHandle());
    }

    for (plUInt32 i = allObjects.GetCount(); i-- > 0;)
    {
      world.DeleteObjectNow(allObjects[i]);
    }

    world.Update();
  }
}
