#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  using TestComponentBaseManager = plComponentManagerSimple<class TestComponentBase, plComponentUpdateType::Always>;

  class TestComponentBase : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestComponentBase, plComponent, TestComponentBaseManager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentBase::s_iUpdateCounter = 0;

  PLASMA_BEGIN_COMPONENT_TYPE(TestComponentBase, 1, plComponentMode::Static)
  PLASMA_END_COMPONENT_TYPE

  //////////////////////////////////////////////////////////////////////////

  using TestComponentDerived1Manager = plComponentManagerSimple<class TestComponentDerived1, plComponentUpdateType::Always>;

  class TestComponentDerived1 : public TestComponentBase
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestComponentDerived1, TestComponentBase, TestComponentDerived1Manager);

  public:
    void Update() { ++s_iUpdateCounter; }

    static int s_iUpdateCounter;
  };

  int TestComponentDerived1::s_iUpdateCounter = 0;

  PLASMA_BEGIN_COMPONENT_TYPE(TestComponentDerived1, 1, plComponentMode::Static)
  PLASMA_END_COMPONENT_TYPE
} // namespace


PLASMA_CREATE_SIMPLE_TEST(World, DerivedComponents)
{
  plWorldDesc worldDesc("Test");
  plWorld world(worldDesc);
  PLASMA_LOCK(world.GetWriteMarker());

  TestComponentBaseManager* pManagerBase = world.GetOrCreateComponentManager<TestComponentBaseManager>();
  TestComponentDerived1Manager* pManagerDerived1 = world.GetOrCreateComponentManager<TestComponentDerived1Manager>();

  plGameObjectDesc desc;
  plGameObject* pObject;
  plGameObjectHandle hObject = world.CreateObject(desc, pObject);
  PLASMA_TEST_BOOL(!hObject.IsInvalidated());

  plGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponentBase::s_iUpdateCounter = 0;
  TestComponentDerived1::s_iUpdateCounter = 0;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Derived Component Update")
  {
    TestComponentBase* pComponentBase = nullptr;
    plComponentHandle hComponentBase = TestComponentBase::CreateComponent(pObject, pComponentBase);

    TestComponentBase* pTestBase = nullptr;
    PLASMA_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestBase));
    PLASMA_TEST_BOOL(pTestBase == pComponentBase);
    PLASMA_TEST_BOOL(pComponentBase->GetHandle() == hComponentBase);
    PLASMA_TEST_BOOL(pComponentBase->GetOwningManager() == pManagerBase);

    TestComponentDerived1* pComponentDerived1 = nullptr;
    plComponentHandle hComponentDerived1 = TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1);

    TestComponentDerived1* pTestDerived1 = nullptr;
    PLASMA_TEST_BOOL(world.TryGetComponent(hComponentDerived1, pTestDerived1));
    PLASMA_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    PLASMA_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentDerived1);
    PLASMA_TEST_BOOL(pComponentDerived1->GetOwningManager() == pManagerDerived1);

    world.Update();

    PLASMA_TEST_INT(TestComponentBase::s_iUpdateCounter, 1);
    PLASMA_TEST_INT(TestComponentDerived1::s_iUpdateCounter, 1);

    // Get component manager via rtti
    PLASMA_TEST_BOOL(world.GetManagerForComponentType(plGetStaticRTTI<TestComponentBase>()) == pManagerBase);
    PLASMA_TEST_BOOL(world.GetManagerForComponentType(plGetStaticRTTI<TestComponentDerived1>()) == pManagerDerived1);
  }
}
