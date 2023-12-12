#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Singleton.h>

class TestSingleton
{
  PLASMA_DECLARE_SINGLETON(TestSingleton);

public:
  TestSingleton()
    : m_SingletonRegistrar(this)
  {
  }

  plInt32 m_iValue = 41;
};

PLASMA_IMPLEMENT_SINGLETON(TestSingleton);

class SingletonInterface
{
public:
  virtual plInt32 GetValue() = 0;
};

class TestSingletonOfInterface : public SingletonInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(TestSingletonOfInterface, SingletonInterface);

public:
  TestSingletonOfInterface()
    : m_SingletonRegistrar(this)
  {
  }

  virtual plInt32 GetValue() { return 23; }
};

PLASMA_IMPLEMENT_SINGLETON(TestSingletonOfInterface);


PLASMA_CREATE_SIMPLE_TEST(Configuration, Singleton)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Singleton Registration")
  {
    {
      TestSingleton* pSingleton = plSingletonRegistry::GetSingletonInstance<TestSingleton>();
      PLASMA_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingleton g_Singleton;

      {
        TestSingleton* pSingleton = plSingletonRegistry::GetSingletonInstance<TestSingleton>();
        PLASMA_TEST_BOOL(pSingleton == &g_Singleton);
        PLASMA_TEST_INT(pSingleton->m_iValue, 41);
      }
    }

    {
      TestSingleton* pSingleton = plSingletonRegistry::GetSingletonInstance<TestSingleton>();
      PLASMA_TEST_BOOL(pSingleton == nullptr);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Singleton of Interface")
  {
    {
      SingletonInterface* pSingleton = plSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      PLASMA_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface g_Singleton;

      {
        SingletonInterface* pSingleton = plSingletonRegistry::GetSingletonInstance<SingletonInterface>();
        PLASMA_TEST_BOOL(pSingleton == &g_Singleton);
        PLASMA_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = plSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
        PLASMA_TEST_BOOL(pSingleton == &g_Singleton);
        PLASMA_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        SingletonInterface* pSingleton = plSingletonRegistry::GetRequiredSingletonInstance<SingletonInterface>();
        PLASMA_TEST_BOOL(pSingleton == &g_Singleton);
        PLASMA_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = plSingletonRegistry::GetRequiredSingletonInstance<TestSingletonOfInterface>();
        PLASMA_TEST_BOOL(pSingleton == &g_Singleton);
        PLASMA_TEST_INT(pSingleton->GetValue(), 23);
      }
    }

    {
      SingletonInterface* pSingleton = plSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      PLASMA_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface* pSingleton = plSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
      PLASMA_TEST_BOOL(pSingleton == nullptr);
    }
  }
}
