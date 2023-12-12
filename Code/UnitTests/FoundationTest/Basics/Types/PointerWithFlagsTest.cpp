#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/PointerWithFlags.h>

PLASMA_CREATE_SIMPLE_TEST(Basics, PointerWithFlags)
{
  struct Dummy
  {
    float a = 3.0f;
    int b = 7;
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "General")
  {
    plPointerWithFlags<Dummy, 2> ptr;

    PLASMA_TEST_INT(ptr.GetFlags(), 0);
    ptr.SetFlags(3);
    PLASMA_TEST_INT(ptr.GetFlags(), 3);

    PLASMA_TEST_BOOL(ptr == nullptr);
    PLASMA_TEST_BOOL(!ptr);

    PLASMA_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(2);
    PLASMA_TEST_INT(ptr.GetFlags(), 2);

    Dummy d1, d2;
    ptr = &d1;
    d2.a = 4;
    d2.b = 8;

    PLASMA_TEST_BOOL(ptr.GetPtr() == &d1);
    PLASMA_TEST_BOOL(ptr.GetPtr() != &d2);

    PLASMA_TEST_INT(ptr.GetFlags(), 2);
    ptr.SetFlags(1);
    PLASMA_TEST_INT(ptr.GetFlags(), 1);

    PLASMA_TEST_BOOL(ptr == &d1);
    PLASMA_TEST_BOOL(ptr != &d2);
    PLASMA_TEST_BOOL(ptr);


    PLASMA_TEST_FLOAT(ptr->a, 3.0f, 0.0f);
    PLASMA_TEST_INT(ptr->b, 7);

    ptr = &d2;

    PLASMA_TEST_INT(ptr.GetFlags(), 1);
    ptr.SetFlags(3);
    PLASMA_TEST_INT(ptr.GetFlags(), 3);

    PLASMA_TEST_BOOL(ptr != &d1);
    PLASMA_TEST_BOOL(ptr == &d2);
    PLASMA_TEST_BOOL(ptr);

    ptr = nullptr;
    PLASMA_TEST_BOOL(!ptr);
    PLASMA_TEST_BOOL(ptr == nullptr);

    PLASMA_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(0);
    PLASMA_TEST_INT(ptr.GetFlags(), 0);

    plPointerWithFlags<Dummy, 2> ptr2 = ptr;
    PLASMA_TEST_BOOL(ptr == ptr2);

    PLASMA_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    PLASMA_TEST_BOOL(ptr2.GetFlags() == ptr.GetFlags());

    ptr2.SetFlags(3);
    PLASMA_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    PLASMA_TEST_BOOL(ptr2.GetFlags() != ptr.GetFlags());

    // the two Ptrs still compare equal (pointer part is equal, even if flags are different)
    PLASMA_TEST_BOOL(ptr == ptr2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Const ptr")
  {
    plPointerWithFlags<const Dummy, 2> ptr;

    Dummy d1, d2;
    ptr = &d1;

    const Dummy* pD1 = &d1;
    const Dummy* pD2 = &d2;

    PLASMA_TEST_BOOL(ptr.GetPtr() == pD1);
    PLASMA_TEST_BOOL(ptr.GetPtr() != pD2);
  }
}
