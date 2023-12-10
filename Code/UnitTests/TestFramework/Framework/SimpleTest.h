#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/TestBaseClass.h>

class PLASMA_TEST_DLL plSimpleTestGroup : public plTestBaseClass
{
public:
  using SimpleTestFunc = void (*)();

  plSimpleTestGroup(const char* szName)
    : m_szTestName(szName)
  {
  }

  void AddSimpleTest(const char* szName, SimpleTestFunc testFunc);

  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void SetupSubTests() override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;

private:
  struct SimpleTestEntry
  {
    const char* m_szName;
    SimpleTestFunc m_Func;
  };

  const char* m_szTestName;
  std::deque<SimpleTestEntry> m_SimpleTests;
};

class PLASMA_TEST_DLL plRegisterSimpleTestHelper : public plEnumerable<plRegisterSimpleTestHelper>
{
  PLASMA_DECLARE_ENUMERABLE_CLASS(plRegisterSimpleTestHelper);

public:
  plRegisterSimpleTestHelper(plSimpleTestGroup* pTestGroup, const char* szTestName, plSimpleTestGroup::SimpleTestFunc func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func = func;
  }

  void RegisterTest() { m_pTestGroup->AddSimpleTest(m_szTestName, m_Func); }

private:
  plSimpleTestGroup* m_pTestGroup;
  const char* m_szTestName;
  plSimpleTestGroup::SimpleTestFunc m_Func;
};

#define PLASMA_CREATE_SIMPLE_TEST_GROUP(GroupName) plSimpleTestGroup PLASMA_CONCAT(g_SimpleTestGroup__, GroupName)(PLASMA_STRINGIZE(GroupName));

#define PLASMA_CREATE_SIMPLE_TEST(GroupName, TestName)                                                                                                   \
  extern plSimpleTestGroup PLASMA_CONCAT(g_SimpleTestGroup__, GroupName);                                                                                \
  static void plSimpleTestFunction__##GroupName##_##TestName();                                                                                      \
  plRegisterSimpleTestHelper plRegisterSimpleTest__##GroupName##TestName(                                                                            \
    &PLASMA_CONCAT(g_SimpleTestGroup__, GroupName), PLASMA_STRINGIZE(TestName), plSimpleTestFunction__##GroupName##_##TestName);                             \
  static void plSimpleTestFunction__##GroupName##_##TestName()
