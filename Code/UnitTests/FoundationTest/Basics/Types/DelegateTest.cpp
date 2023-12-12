#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

namespace
{
  struct TestType
  {
    TestType(){}; // NOLINT: Allow default construction

    plInt32 MethodWithManyParams(plInt32 a, plInt32 b, plInt32 c, plInt32 d, plInt32 e, plInt32 f) { return m_iA + a + b + c + d + e + f; }

    plInt32 Method(plInt32 b) { return b + m_iA; }

    plInt32 ConstMethod(plInt32 b) const { return b + m_iA + 4; }

    virtual plInt32 VirtualMethod(plInt32 b) { return b; }

    mutable plInt32 m_iA;
  };

  struct TestTypeDerived : public TestType
  {
    plInt32 Method(plInt32 b) { return b + 4; }

    virtual plInt32 VirtualMethod(plInt32 b) override { return b + 43; }
  };

  struct BaseA
  {
    virtual ~BaseA() = default;
    virtual void bar() {}

    int m_i1;
  };

  struct BaseB
  {
    virtual ~BaseB() = default;
    virtual void foo() {}
    int m_i2;
  };

  struct ComplexClass : public BaseA, public BaseB
  {
    ComplexClass() { m_ctorDel = plMakeDelegate(&ComplexClass::nonVirtualFunc, this); }

    virtual ~ComplexClass()
    {
      m_dtorDel = plMakeDelegate(&ComplexClass::nonVirtualFunc, this);
      PLASMA_TEST_BOOL(m_ctorDel.IsEqualIfComparable(m_dtorDel));
    }
    virtual void bar() override {}
    virtual void foo() override {}



    void nonVirtualFunc()
    {
      m_i1 = 1;
      m_i2 = 2;
      m_i3 = 3;
    }

    int m_i3;

    plDelegate<void()> m_ctorDel;
    plDelegate<void()> m_dtorDel;
  };

  static plInt32 Function(plInt32 b) { return b + 2; }
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Basics, Delegate)
{
  using TestDelegate = plDelegate<plInt32(plInt32)>;
  TestDelegate d;

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
  PLASMA_TEST_BOOL(sizeof(d) == 32);
#endif

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Method")
  {
    TestTypeDerived test;
    test.m_iA = 42;

    d = TestDelegate(&TestType::Method, &test);
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::Method, &test)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::Method, &test)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 8);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Method With Many Params")
  {
    using TestDelegateMany = plDelegate<plInt32(plInt32, plInt32, plInt32, plInt32, plInt32, plInt32)>;
    TestDelegateMany many;

    TestType test;
    test.m_iA = 1000000;

    many = TestDelegateMany(&TestType::MethodWithManyParams, &test);
    PLASMA_TEST_BOOL(many.IsEqualIfComparable(TestDelegateMany(&TestType::MethodWithManyParams, &test)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(many(1, 10, 100, 1000, 10000, 100000), 1111111);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Complex Class")
  {
    ComplexClass* c = new ComplexClass();
    delete c;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Const Method")
  {
    const TestType constTest;
    constTest.m_iA = 35;

    d = TestDelegate(&TestType::ConstMethod, &constTest);
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::ConstMethod, &constTest)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 43);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::VirtualMethod, &test)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::VirtualMethod, &test)));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 47);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Function")
  {
    d = &Function;
    PLASMA_TEST_BOOL(d.IsEqualIfComparable(&Function));
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(4), 6);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - no capture")
  {
    d = [](plInt32 i) { return i * 4; };
    PLASMA_TEST_BOOL(d.IsComparable());
    PLASMA_TEST_INT(d(2), 8);

    TestDelegate d2 = d;
    PLASMA_TEST_BOOL(d2.IsEqualIfComparable(d));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture by value")
  {
    plInt32 c = 20;
    d = [c](plInt32) { return c; };
    PLASMA_TEST_BOOL(!d.IsComparable());
    PLASMA_TEST_INT(d(3), 20);
    c = 10;
    PLASMA_TEST_INT(d(3), 20);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture by value, mutable")
  {
    plInt32 c = 20;
    d = [c](plInt32) mutable { return c; };
    PLASMA_TEST_BOOL(!d.IsComparable());
    PLASMA_TEST_INT(d(3), 20);
    c = 10;
    PLASMA_TEST_INT(d(3), 20);

    d = [c](plInt32 b) mutable -> decltype(b + c) {
      auto result = b + c;
      c = 1;
      return result;
    };
    PLASMA_TEST_BOOL(!d.IsComparable());
    PLASMA_TEST_INT(d(3), 13);
    PLASMA_TEST_INT(d(3), 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture by reference")
  {
    plInt32 c = 20;
    d = [&c](plInt32 i) -> decltype(i) {
      c = 5;
      return i;
    };
    PLASMA_TEST_BOOL(!d.IsComparable());
    PLASMA_TEST_INT(d(3), 3);
    PLASMA_TEST_INT(c, 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture by value of non-pod")
  {
    struct RefCountedInt : public plRefCounted
    {
      RefCountedInt() = default;
      RefCountedInt(int i)
        : m_value(i)
      {
      }
      int m_value;
    };

    plSharedPtr<RefCountedInt> shared = PLASMA_DEFAULT_NEW(RefCountedInt, 1);
    PLASMA_TEST_INT(shared->GetRefCount(), 1);
    {
      TestDelegate deleteMe = [shared](plInt32 i) -> decltype(i) { return 0; };
      PLASMA_TEST_BOOL(!deleteMe.IsComparable());
      PLASMA_TEST_INT(shared->GetRefCount(), 2);
    }
    PLASMA_TEST_INT(shared->GetRefCount(), 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture lots of things")
  {
    plInt64 a = 10;
    plInt64 b = 20;
    plInt64 c = 30;
    d = [a, b, c](plInt32 i) -> plInt32 { return static_cast<plInt32>(a + b + c + i); };
    PLASMA_TEST_INT(d(6), 66);
    PLASMA_TEST_BOOL(!d.IsComparable());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture lots of things - custom allocator")
  {
    plInt64 a = 10;
    plInt64 b = 20;
    plInt64 c = 30;
    d = TestDelegate([a, b, c](plInt32 i) -> plInt32 { return static_cast<plInt32>(a + b + c + i); }, plFoundation::GetAlignedAllocator());
    PLASMA_TEST_INT(d(6), 66);
    PLASMA_TEST_BOOL(!d.IsComparable());

    d.Invalidate();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move semantics")
  {
    // Move pure function
    {
      d.Invalidate();
      TestDelegate d2 = &Function;
      d = std::move(d2);
      PLASMA_TEST_BOOL(d.IsValid());
      PLASMA_TEST_BOOL(!d2.IsValid());
      PLASMA_TEST_BOOL(d.IsComparable());
      PLASMA_TEST_INT(d(4), 6);
    }

    // Move delegate
    plConstructionCounter::Reset();
    d.Invalidate();
    {
      plConstructionCounter value;
      value.m_iData = 666;
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 1);
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = [value](plInt32 i) -> plInt32 { return value.m_iData; };
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = std::move(d2);
      // Moving a construction counter also counts as construction
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 4);
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 1);
      PLASMA_TEST_BOOL(d.IsValid());
      PLASMA_TEST_BOOL(!d2.IsValid());
      PLASMA_TEST_BOOL(!d.IsComparable());
      PLASMA_TEST_INT(d(0), 666);
    }
    PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 2); // value out of scope
    PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 3); // lambda destroyed.
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - Copy")
  {
    d.Invalidate();
    plConstructionCounter::Reset();
    {
      plConstructionCounter value;
      value.m_iData = 666;
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 1);
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = TestDelegate([value](plInt32 i) -> plInt32 { return value.m_iData; }, plFoundation::GetAlignedAllocator());
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = d2;
      PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 4); // Lambda Copy
      PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 1);
      PLASMA_TEST_BOOL(d.IsValid());
      PLASMA_TEST_BOOL(d2.IsValid());
      PLASMA_TEST_BOOL(!d.IsComparable());
      PLASMA_TEST_BOOL(!d2.IsComparable());
      PLASMA_TEST_INT(d(0), 666);
      PLASMA_TEST_INT(d2(0), 666);
    }
    PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 3); // value and lambda out of scope
    PLASMA_TEST_INT(plConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    PLASMA_TEST_INT(plConstructionCounter::s_iDestructions, 4); // lambda destroyed.
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lambda - capture non-copyable type")
  {
    plUniquePtr<plConstructionCounter> data(PLASMA_DEFAULT_NEW(plConstructionCounter));
    data->m_iData = 666;
    TestDelegate d2 = [data = std::move(data)](plInt32 i) -> plInt32 { return data->m_iData; };
    PLASMA_TEST_INT(d2(0), 666);
    d = std::move(d2);
    PLASMA_TEST_BOOL(d.IsValid());
    PLASMA_TEST_BOOL(!d2.IsValid());
    PLASMA_TEST_INT(d(0), 666);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMakeDelegate")
  {
    auto d1 = plMakeDelegate(&Function);
    PLASMA_TEST_BOOL(d1.IsEqualIfComparable(plMakeDelegate(&Function)));

    TestType instance;
    auto d2 = plMakeDelegate(&TestType::Method, &instance);
    PLASMA_TEST_BOOL(d2.IsEqualIfComparable(plMakeDelegate(&TestType::Method, &instance)));
    auto d3 = plMakeDelegate(&TestType::ConstMethod, &instance);
    PLASMA_TEST_BOOL(d3.IsEqualIfComparable(plMakeDelegate(&TestType::ConstMethod, &instance)));
    auto d4 = plMakeDelegate(&TestType::VirtualMethod, &instance);
    PLASMA_TEST_BOOL(d4.IsEqualIfComparable(plMakeDelegate(&TestType::VirtualMethod, &instance)));

    TestType instance2;
    auto d2_2 = plMakeDelegate(&TestType::Method, &instance2);
    PLASMA_TEST_BOOL(!d2_2.IsEqualIfComparable(d2));

    PLASMA_IGNORE_UNUSED(d1);
    PLASMA_IGNORE_UNUSED(d2);
    PLASMA_IGNORE_UNUSED(d2_2);
    PLASMA_IGNORE_UNUSED(d3);
    PLASMA_IGNORE_UNUSED(d4);
  }
}
