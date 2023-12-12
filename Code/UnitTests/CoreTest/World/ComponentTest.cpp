#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public plComponentManager<TestComponent, plBlockStorageType::FreeList>
  {
  public:
    TestComponentManager(plWorld* pWorld)
      : plComponentManager<TestComponent, plBlockStorageType::FreeList>(pWorld)
    {
    }

    virtual void Initialize() override
    {
      auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2 = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      auto desc3 = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update3, this);
      desc3.m_fPriority = 1000.0f;

      auto desc4 = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::AUpdate3, this);
      desc4.m_fPriority = 1000.0f;

      desc.m_DependsOn.PushBack(plMakeHashedString("TestComponentManager::Update2")); // update2 will be called before update
      desc.m_DependsOn.PushBack(plMakeHashedString("TestComponentManager::Update3")); // update3 will be called before update

      auto descAsync = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase = plComponentManagerBase::UpdateFunctionDesc::Phase::Async;
      descAsync.m_uiGranularity = 20;

      // Update functions are now registered in reverse order, so we can test whether dependencies work.
      this->RegisterUpdateFunction(descAsync);
      this->RegisterUpdateFunction(desc4);
      this->RegisterUpdateFunction(desc3);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(desc);
    }

    void Update(const plWorldModule::UpdateContext& context);
    void Update2(const plWorldModule::UpdateContext& context);
    void Update3(const plWorldModule::UpdateContext& context);
    void AUpdate3(const plWorldModule::UpdateContext& context);
    void UpdateAsync(const plWorldModule::UpdateContext& context);
  };

  class TestComponent : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestComponent, plComponent, TestComponentManager);

  public:
    TestComponent()

      = default;
    ~TestComponent() = default;

    virtual void Initialize() override { ++s_iInitCounter; }

    virtual void Deinitialize() override { --s_iInitCounter; }

    virtual void OnActivated() override
    {
      ++s_iActivateCounter;

      SpawnOther();
    }

    virtual void OnDeactivated() override { --s_iActivateCounter; }

    virtual void OnSimulationStarted() override { ++s_iSimulationStartedCounter; }

    void Update() { m_iSomeData *= 5; }

    void Update2() { m_iSomeData += 3; }

    void SpawnOther();

    plInt32 m_iSomeData = 1;

    static plInt32 s_iInitCounter;
    static plInt32 s_iActivateCounter;
    static plInt32 s_iSimulationStartedCounter;

    static bool s_bSpawnOther;
  };

  plInt32 TestComponent::s_iInitCounter = 0;
  plInt32 TestComponent::s_iActivateCounter = 0;
  plInt32 TestComponent::s_iSimulationStartedCounter = 0;
  bool TestComponent::s_bSpawnOther = false;

  PLASMA_BEGIN_COMPONENT_TYPE(TestComponent, 1, plComponentMode::Static)
  PLASMA_END_COMPONENT_TYPE

  void TestComponentManager::Update(const plWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(const plWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::Update3(const plWorldModule::UpdateContext& context) {}

  void TestComponentManager::AUpdate3(const plWorldModule::UpdateContext& context) {}

  void TestComponentManager::UpdateAsync(const plWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  using TestComponent2Manager = plComponentManager<class TestComponent2, plBlockStorageType::FreeList>;

  class TestComponent2 : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestComponent2, plComponent, TestComponent2Manager);

    virtual void OnActivated() override { TestComponent::s_iActivateCounter++; }
  };

  PLASMA_BEGIN_COMPONENT_TYPE(TestComponent2, 1, plComponentMode::Static)
  PLASMA_END_COMPONENT_TYPE

  void TestComponent::SpawnOther()
  {
    if (s_bSpawnOther)
    {
      plGameObjectDesc desc;
      desc.m_hParent = GetOwner()->GetHandle();

      plGameObject* pChild = nullptr;
      GetWorld()->CreateObject(desc, pChild);

      TestComponent2* pChildComponent = nullptr;
      TestComponent2::CreateComponent(pChild, pChildComponent);
    }
  }
} // namespace


PLASMA_CREATE_SIMPLE_TEST(World, Components)
{
  plWorldDesc worldDesc("Test");
  plWorld world(worldDesc);
  PLASMA_LOCK(world.GetWriteMarker());

  TestComponentManager* pManager = world.GetOrCreateComponentManager<TestComponentManager>();

  plGameObject* pTestObject1;
  plGameObject* pTestObject2;

  {
    plGameObjectDesc desc;
    plGameObjectHandle hObject = world.CreateObject(desc, pTestObject1);
    PLASMA_TEST_BOOL(!hObject.IsInvalidated());
    world.CreateObject(desc, pTestObject2);
  }

  TestComponent* pTestComponent = nullptr;

  TestComponent::s_iInitCounter = 0;
  TestComponent::s_iActivateCounter = 0;
  TestComponent::s_iSimulationStartedCounter = 0;
  TestComponent::s_bSpawnOther = false;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Component Init")
  {
    // test recursive write lock
    PLASMA_LOCK(world.GetWriteMarker());

    plComponentHandle handle;
    PLASMA_TEST_BOOL(!world.TryGetComponent(handle, pTestComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(pTestObject1, pTestComponent);

    TestComponent* pTest = nullptr;
    PLASMA_TEST_BOOL(world.TryGetComponent(handle, pTest));
    PLASMA_TEST_BOOL(pTest == pTestComponent);
    PLASMA_TEST_BOOL(pTestComponent->GetHandle() == handle);

    TestComponent2* pTest2 = nullptr;
    PLASMA_TEST_BOOL(!world.TryGetComponent(handle, pTest2));

    PLASMA_TEST_INT(pTestComponent->m_iSomeData, 1);
    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);

    for (plUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pTestObject2, pTestComponent);
      pTestComponent->m_iSomeData = i + 1;
    }

    PLASMA_TEST_INT(pManager->GetComponentCount(), 100);
    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);

    // Update with components created
    world.Update();

    PLASMA_TEST_INT(pManager->GetComponentCount(), 100);
    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Component Update")
  {
    // test recursive read lock
    PLASMA_LOCK(world.GetReadMarker());

    world.Update();

    plUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(it->m_iSomeData, (((uiCounter + 4) * 25) + 3) * 25);
      ++uiCounter;
    }

    PLASMA_TEST_INT(uiCounter, 100);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pTestComponent->GetHandle());
    PLASMA_TEST_INT(pManager->GetComponentCount(), 99);
    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 99);

    // component should also be removed from the game object
    PLASMA_TEST_INT(pTestObject2->GetComponents().GetCount(), 98);

    world.DeleteObjectNow(pTestObject2->GetHandle());
    world.Update();

    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);

    world.DeleteComponentManager<TestComponentManager>();
    pManager = nullptr;
    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Delete Objects with Component")
  {
    plGameObjectDesc desc;

    plGameObject* pObjectA = nullptr;
    plGameObject* pObjectB = nullptr;
    plGameObject* pObjectC = nullptr;

    desc.m_sName.Assign("A");
    plGameObjectHandle hObjectA = world.CreateObject(desc, pObjectA);
    desc.m_sName.Assign("B");
    plGameObjectHandle hObjectB = world.CreateObject(desc, pObjectB);
    desc.m_sName.Assign("C");
    plGameObjectHandle hObjectC = world.CreateObject(desc, pObjectC);

    PLASMA_TEST_BOOL(!hObjectA.IsInvalidated());
    PLASMA_TEST_BOOL(!hObjectB.IsInvalidated());
    PLASMA_TEST_BOOL(!hObjectC.IsInvalidated());

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    plComponentHandle hComponentA = TestComponent::CreateComponent(pObjectA, pComponentA);
    plComponentHandle hComponentB = TestComponent::CreateComponent(pObjectB, pComponentB);
    plComponentHandle hComponentC = TestComponent::CreateComponent(pObjectC, pComponentC);

    PLASMA_TEST_BOOL(!hComponentA.IsInvalidated());
    PLASMA_TEST_BOOL(!hComponentB.IsInvalidated());
    PLASMA_TEST_BOOL(!hComponentC.IsInvalidated());

    world.DeleteObjectNow(pObjectB->GetHandle());

    PLASMA_TEST_BOOL(pObjectA->IsActive());
    PLASMA_TEST_BOOL(pComponentA->IsActive());
    PLASMA_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    PLASMA_TEST_BOOL(!pObjectB->IsActive());
    PLASMA_TEST_BOOL(!pComponentB->IsActive());
    PLASMA_TEST_BOOL(pComponentB->GetOwner() == nullptr);

    PLASMA_TEST_BOOL(pObjectC->IsActive());
    PLASMA_TEST_BOOL(pComponentC->IsActive());
    PLASMA_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    world.Update();

    PLASMA_TEST_BOOL(world.TryGetObject(hObjectA, pObjectA));
    PLASMA_TEST_BOOL(world.TryGetObject(hObjectC, pObjectC));

    // Since we're not recompacting storage for components, pointer should still be valid.
    // PLASMA_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    // PLASMA_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    PLASMA_TEST_BOOL(pObjectA->IsActive());
    PLASMA_TEST_BOOL(pObjectA->GetName() == "A");
    PLASMA_TEST_BOOL(pComponentA->IsActive());
    PLASMA_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    PLASMA_TEST_BOOL(pObjectC->IsActive());
    PLASMA_TEST_BOOL(pObjectC->GetName() == "C");
    PLASMA_TEST_BOOL(pComponentC->IsActive());
    PLASMA_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent* pComponentB2 = nullptr;
    plComponentHandle hComponentB2 = TestComponent::CreateComponent(pObjectB, pComponentB2);
    PLASMA_TEST_BOOL(!hComponentB2.IsInvalidated());
    PLASMA_TEST_BOOL(pComponentB2 == pComponentB);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Get Components")
  {
    const plWorld& constWorld = world;

    const TestComponentManager* pConstManager = constWorld.GetComponentManager<TestComponentManager>();

    for (auto it = pConstManager->GetComponents(); it.IsValid(); it.Next())
    {
      plComponentHandle hComponent = it->GetHandle();

      const TestComponent* pConstComponent = nullptr;
      PLASMA_TEST_BOOL(constWorld.TryGetComponent(hComponent, pConstComponent));
      PLASMA_TEST_BOOL(pConstComponent == (const TestComponent*)it);

      PLASMA_TEST_BOOL(pConstManager->TryGetComponent(hComponent, pConstComponent));
      PLASMA_TEST_BOOL(pConstComponent == (const TestComponent*)it);
    }

    world.DeleteComponentManager<TestComponentManager>();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Component Callbacks")
  {
    plGameObjectDesc desc;
    plGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    // Simulation stopped, component active
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(false);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActiveFlag(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 2);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation stopped, component inactive
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(false);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation started, component active
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation started, component inactive
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActiveFlag(false);

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 0);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActiveFlag(true);
      world.Update();

      PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 1);
      PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Component dependent initialization")
  {
    plGameObjectDesc desc;
    plGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    world.SetWorldSimulationEnabled(true);

    TestComponent::s_iInitCounter = 0;
    TestComponent::s_iActivateCounter = 0;
    TestComponent::s_iSimulationStartedCounter = 0;
    TestComponent::s_bSpawnOther = true;

    TestComponent* pComponent = nullptr;
    TestComponent::CreateComponent(pObject, pComponent);

    world.Update();

    PLASMA_TEST_INT(TestComponent::s_iInitCounter, 1);
    PLASMA_TEST_INT(TestComponent::s_iActivateCounter, 2);
    PLASMA_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);
  }
}
