#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/RefCounted.h>

class RefCountedTestClass : public plRefCounted
{
public:
  plUInt32 m_uiDummyMember = 0x42u;
};

PLASMA_CREATE_SIMPLE_TEST(Basics, RefCounted)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ref Counting")
  {
    RefCountedTestClass Instance;

    PLASMA_TEST_BOOL(Instance.GetRefCount() == 0);
    PLASMA_TEST_BOOL(!Instance.IsReferenced());

    Instance.AddRef();

    PLASMA_TEST_BOOL(Instance.GetRefCount() == 1);
    PLASMA_TEST_BOOL(Instance.IsReferenced());

    /// Test scoped ref pointer
    {
      plScopedRefPointer<RefCountedTestClass> ScopeTester(&Instance);

      PLASMA_TEST_BOOL(Instance.GetRefCount() == 2);
      PLASMA_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test assignment of scoped ref pointer
    {
      plScopedRefPointer<RefCountedTestClass> ScopeTester;

      ScopeTester = &Instance;

      PLASMA_TEST_BOOL(Instance.GetRefCount() == 2);
      PLASMA_TEST_BOOL(Instance.IsReferenced());

      plScopedRefPointer<RefCountedTestClass> ScopeTester2;

      ScopeTester2 = ScopeTester;

      PLASMA_TEST_BOOL(Instance.GetRefCount() == 3);
      PLASMA_TEST_BOOL(Instance.IsReferenced());

      plScopedRefPointer<RefCountedTestClass> ScopeTester3(ScopeTester);

      PLASMA_TEST_BOOL(Instance.GetRefCount() == 4);
      PLASMA_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test copy constructor for plRefCounted
    {
      RefCountedTestClass inst2(Instance);
      RefCountedTestClass inst3;
      inst3 = Instance;

      PLASMA_TEST_BOOL(Instance.GetRefCount() == 1);
      PLASMA_TEST_BOOL(Instance.IsReferenced());

      PLASMA_TEST_BOOL(inst2.GetRefCount() == 0);
      PLASMA_TEST_BOOL(!inst2.IsReferenced());

      PLASMA_TEST_BOOL(inst3.GetRefCount() == 0);
      PLASMA_TEST_BOOL(!inst3.IsReferenced());
    }

    PLASMA_TEST_BOOL(Instance.GetRefCount() == 1);
    PLASMA_TEST_BOOL(Instance.IsReferenced());

    Instance.ReleaseRef();

    PLASMA_TEST_BOOL(Instance.GetRefCount() == 0);
    PLASMA_TEST_BOOL(!Instance.IsReferenced());
  }
}
