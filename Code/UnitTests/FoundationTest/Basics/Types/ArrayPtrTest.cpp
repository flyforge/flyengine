#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

template <typename T>
static void testArrayPtr(plArrayPtr<T> arrayPtr, typename plArrayPtr<T>::PointerType extectedPtr, plUInt32 uiExpectedCount)
{
  PLASMA_TEST_BOOL(arrayPtr.GetPtr() == extectedPtr);
  PLASMA_TEST_INT(arrayPtr.GetCount(), uiExpectedCount);
}

// static void TakeConstArrayPtr(plArrayPtr<const int> cint)
//{
//}
//
// static void TakeConstArrayPtr2(plArrayPtr<const int*> cint, plArrayPtr<const int* const> cintc)
//{
//}

PLASMA_CREATE_SIMPLE_TEST(Basics, ArrayPtr)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Empty Constructor")
  {
    plArrayPtr<plInt32> Empty;

    PLASMA_TEST_BOOL(Empty.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(Empty.GetCount() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    plArrayPtr<plInt32> ap2(pIntData, 0u);
    PLASMA_TEST_BOOL(ap2.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(ap2.GetCount() == 0);

    plArrayPtr<plInt32> ap3(pIntData);
    PLASMA_TEST_BOOL(ap3.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap3.GetCount() == 5);

    plArrayPtr<plInt32> ap4(ap);
    PLASMA_TEST_BOOL(ap4.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap4.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMakeArrayPtr")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    testArrayPtr(plMakeArrayPtr(pIntData, 3), pIntData, 3);
    testArrayPtr(plMakeArrayPtr(pIntData, 0), nullptr, 0);
    testArrayPtr(plMakeArrayPtr(pIntData), pIntData, 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    plArrayPtr<plInt32> ap2;
    ap2 = ap;

    PLASMA_TEST_BOOL(ap2.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap2.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    PLASMA_TEST_BOOL(ap.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(ap.GetCount() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== / operator!= / operator<")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap1(pIntData, 3);
    plArrayPtr<plInt32> ap2(pIntData, 3);
    plArrayPtr<plInt32> ap3(pIntData, 4);
    plArrayPtr<plInt32> ap4(pIntData + 1, 3);

    PLASMA_TEST_BOOL(ap1 == ap2);
    PLASMA_TEST_BOOL(ap1 != ap3);
    PLASMA_TEST_BOOL(ap1 != ap4);

    PLASMA_TEST_BOOL(ap1 < ap3);
    plInt32 pIntData2[] = {1, 2, 4};
    plArrayPtr<plInt32> ap5(pIntData2, 3);
    PLASMA_TEST_BOOL(ap1 < ap5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator[]")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap(pIntData + 1, 3);
    PLASMA_TEST_INT(ap[0], 2);
    PLASMA_TEST_INT(ap[1], 3);
    PLASMA_TEST_INT(ap[2], 4);
    ap[2] = 10;
    PLASMA_TEST_INT(ap[2], 10);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "const operator[]")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    const plArrayPtr<plInt32> ap(pIntData + 1, 3);
    PLASMA_TEST_INT(ap[0], 2);
    PLASMA_TEST_INT(ap[1], 3);
    PLASMA_TEST_INT(ap[2], 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CopyFrom")
  {
    plInt32 pIntData1[] = {1, 2, 3, 4, 5};
    plInt32 pIntData2[] = {6, 7, 8, 9, 0};

    plArrayPtr<plInt32> ap1(pIntData1 + 1, 3);
    plArrayPtr<plInt32> ap2(pIntData2 + 2, 3);

    ap1.CopyFrom(ap2);

    PLASMA_TEST_INT(pIntData1[0], 1);
    PLASMA_TEST_INT(pIntData1[1], 8);
    PLASMA_TEST_INT(pIntData1[2], 9);
    PLASMA_TEST_INT(pIntData1[3], 0);
    PLASMA_TEST_INT(pIntData1[4], 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetSubArray")
  {
    plInt32 pIntData1[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap1(pIntData1, 5);
    plArrayPtr<plInt32> ap2 = ap1.GetSubArray(2, 3);

    PLASMA_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    PLASMA_TEST_BOOL(ap2.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Const Conversions")
  {
    plInt32 pIntData1[] = {1, 2, 3, 4, 5};
    plArrayPtr<plInt32> ap1(pIntData1);
    plArrayPtr<const plInt32> ap2(ap1);
    plArrayPtr<const plInt32> ap3(pIntData1);
    ap2 = ap1; // non const to const assign
    ap3 = ap2; // const to const assign
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Empty Constructor (const)")
  {
    plArrayPtr<const plInt32> Empty;

    PLASMA_TEST_BOOL(Empty.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(Empty.GetCount() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (const)")
  {
    const plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<const plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    plArrayPtr<const plInt32> ap2(pIntData, 0u);
    PLASMA_TEST_BOOL(ap2.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(ap2.GetCount() == 0);

    plArrayPtr<const plInt32> ap3(pIntData);
    PLASMA_TEST_BOOL(ap3.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap3.GetCount() == 5);

    plArrayPtr<const plInt32> ap4(ap);
    PLASMA_TEST_BOOL(ap4.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap4.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=  (const)")
  {
    const plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<const plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    plArrayPtr<const plInt32> ap2;
    ap2 = ap;

    PLASMA_TEST_BOOL(ap2.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap2.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear (const)")
  {
    const plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<const plInt32> ap(pIntData, 3);
    PLASMA_TEST_BOOL(ap.GetPtr() == pIntData);
    PLASMA_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    PLASMA_TEST_BOOL(ap.GetPtr() == nullptr);
    PLASMA_TEST_BOOL(ap.GetCount() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== / operator!=  (const)")
  {
    plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<plInt32> ap1(pIntData, 3);
    plArrayPtr<const plInt32> ap2(pIntData, 3);
    plArrayPtr<const plInt32> ap3(pIntData, 4);
    plArrayPtr<const plInt32> ap4(pIntData + 1, 3);

    PLASMA_TEST_BOOL(ap1 == ap2);
    PLASMA_TEST_BOOL(ap3 != ap1);
    PLASMA_TEST_BOOL(ap1 != ap4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator[]  (const)")
  {
    const plInt32 pIntData[] = {1, 2, 3, 4, 5};

    plArrayPtr<const plInt32> ap(pIntData + 1, 3);
    PLASMA_TEST_INT(ap[0], 2);
    PLASMA_TEST_INT(ap[1], 3);
    PLASMA_TEST_INT(ap[2], 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "const operator[] (const)")
  {
    const plInt32 pIntData[] = {1, 2, 3, 4, 5};

    const plArrayPtr<const plInt32> ap(pIntData + 1, 3);
    PLASMA_TEST_INT(ap[0], 2);
    PLASMA_TEST_INT(ap[1], 3);
    PLASMA_TEST_INT(ap[2], 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetSubArray (const)")
  {
    const plInt32 pIntData1[] = {1, 2, 3, 4, 5};

    plArrayPtr<const plInt32> ap1(pIntData1, 5);
    plArrayPtr<const plInt32> ap2 = ap1.GetSubArray(2, 3);

    PLASMA_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    PLASMA_TEST_BOOL(ap2.GetCount() == 3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
  {
    plDynamicArray<plInt32> a1;

    for (plInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    plArrayPtr<plInt32> ptr1 = a1;

    // STL sort
    std::sort(begin(ptr1), end(ptr1));

    for (plInt32 i = 1; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(ptr1[i - 1] <= ptr1[i]);
    }

    // foreach
    plUInt32 prev = 0;
    for (plUInt32 val : ptr1)
    {
      PLASMA_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const plDynamicArray<plInt32>& a2 = a1;

    const plArrayPtr<const plInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(begin(ptr2), end(ptr2), 400);
    PLASMA_TEST_BOOL(*lb == ptr2[400]);
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
  {
    plDynamicArray<plInt32> a1;

    for (plInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    plArrayPtr<plInt32> ptr1 = a1;

    // STL sort
    std::sort(rbegin(ptr1), rend(ptr1));

    for (plInt32 i = 1; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(ptr1[i - 1] >= ptr1[i]);
    }

    // foreach
    plUInt32 prev = 1000;
    for (plUInt32 val : ptr1)
    {
      PLASMA_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const plDynamicArray<plInt32>& a2 = a1;

    const plArrayPtr<const plInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(ptr2), rend(ptr2), 400);
    PLASMA_TEST_BOOL(*lb == ptr2[1000 - 400 - 1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plDynamicArray<plInt32> a0;
    plArrayPtr<plInt32> a1 = a0;

    for (plInt32 i = -100; i < 100; ++i)
      PLASMA_TEST_BOOL(!a1.Contains(i));

    for (plInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);
    for (plInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);

    a1 = a0;

    for (plInt32 i = 0; i < 100; ++i)
    {
      PLASMA_TEST_BOOL(a1.Contains(i));
      PLASMA_TEST_INT(a1.IndexOf(i), i);
      PLASMA_TEST_INT(a1.IndexOf(i, 100), i + 100);
      PLASMA_TEST_INT(a1.LastIndexOf(i), i + 100);
      PLASMA_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  // "Implicit Conversions"
  //{
  //  {
  //    plHybridArray<int, 4> data;
  //    TakeConstArrayPtr(data);
  //    TakeConstArrayPtr(data.GetArrayPtr());
  //  }
  //  {
  //    plHybridArray<int*, 4> data;
  //    //TakeConstArrayPtr2(data, data); // does not compile
  //    TakeConstArrayPtr2(data.GetArrayPtr(), data.GetArrayPtr());
  //  }
  //}
}
